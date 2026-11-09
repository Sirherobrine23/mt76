// SPDX-License-Identifier: GPL-2.0-only
/* Airoha legacy NPU receive transport for PCIe MT7915/MT7916 radios. */
#include <linux/debugfs.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/seq_file.h>
#include <linux/pci.h>
#include <linux/regmap.h>
#include <linux/sizes.h>

#include "mt7915.h"
#include "mac.h"

#define MT7915_NPU_DESC_NUM	512
#define MT7915_NPU_BUF_SIZE	4096
#define MT7915_NPU_BA_SIZE	SZ_2M
#define MT7915_NPU_RX_DONE	NPU_RX_DMA_DESC_DONE_MASK
/* Clanker stores the total in the low field and the fragment in the high field. */
#define MT7915_NPU_RX_CUR_LEN	NPU_RX_DMA_DESC_LEN_MASK
#define MT7915_NPU_RX_LEN	NPU_RX_DMA_DESC_CUR_LEN_MASK
#define MT7915_NPU_RX_LAST	NPU_RX_DMA_DESC_LAST_MASK
#define MT7915_NPU_RX_PART	GENMASK(28, 26)
#define MT7915_NPU_RX_PARTS	GENMASK(31, 29)
#define MT7915_NPU_RX_REORDERED	BIT(31)
#define MT7915_NPU_RX_PN_VALID	BIT(30)
#define MT7915_NPU_RX_BYPASS_BA	BIT(29)
#define MT7915_NPU_RX_PPE		BIT(28)
#define MT7915_NPU_STA_MAX	150

/* ClankerNPU host receive descriptor, 24 bytes on the wire. */
struct mt7915_npu_desc {
	u32 ctrl;
	u32 info;
	u32 vendor;
	u32 addr;
	u64 cookie;
} __packed;

struct mt7915_npu_queue {
	struct mt7915_dev *dev;
	struct napi_struct napi;
	struct mt7915_npu_desc *desc;
	dma_addr_t desc_dma;
	void *buf;
	dma_addr_t buf_dma;
	struct sk_buff *partial;
	u32 regs;
	u32 radio_regs;
	u32 radio_desc;
	u16 head;
	u16 total;
	u8 part;
	u8 band;
	u8 local_band;
	int irq;
	bool napi_added;
	bool irq_added;
	bool napi_enabled;
	atomic64_t interrupts;
	atomic64_t descriptors;
	atomic64_t packets;
};

struct mt7915_npu {
	struct airoha_npu *npu;
	struct net_device *napi_dev;
	struct mt7915_npu_queue q[2];
	void *ba;
	dma_addr_t ba_dma;
	bool published;
	bool started;
	bool retired;
	bool direct;
	u8 band_mask;
	u8 queue_mask;
	u8 port;
	u8 model;
};

/* A DBDC radio needs both channels; independent radios need one each. */
static DEFINE_MUTEX(mt7915_npu_lock);
static unsigned long mt7915_npu_queues;
/* The legacy firmware exposes one BA spill pool for both channels. */
static void *mt7915_npu_ba_pool;
static dma_addr_t mt7915_npu_ba_dma;
static unsigned int mt7915_npu_ba_users;
static bool mt7915_npu_memory_initialized;

static int mt7915_npu_send_data(struct mt7915_dev *dev, unsigned int band,
			       enum airoha_npu_wlan_set_cmd cmd,
			       void *data, size_t len)
{
	if (band >= ARRAY_SIZE(dev->npu->q) ||
	    !(dev->npu->band_mask & BIT(band)))
		return -EINVAL;

	return airoha_npu_wlan_send_msg(dev->npu->npu,
				      dev->npu->q[band].band, cmd,
				      data, len, GFP_KERNEL);
}

bool mt7915_npu_active(struct mt7915_dev *dev)
{
	return !!dev->npu;
}

static int mt7915_npu_send(struct mt7915_dev *dev, unsigned int band,
			   enum airoha_npu_wlan_set_cmd cmd, u32 val,
			   gfp_t gfp)
{
	if (band >= ARRAY_SIZE(dev->npu->q) ||
	    !(dev->npu->band_mask & BIT(band)))
		return -EINVAL;
	return airoha_npu_wlan_send_msg(dev->npu->npu,
				      dev->npu->q[band].band, cmd,
				      &val, sizeof(val), gfp);
}

static void mt7915_npu_drop_partial(struct mt7915_npu_queue *q)
{
	dev_kfree_skb_any(q->partial);
	q->partial = NULL;
	q->part = 0;
	q->total = 0;
}

/* Normal packets may be interleaved between the fragments of a large packet. */
static struct sk_buff *mt7915_npu_rx(struct mt7915_npu_queue *q, u32 ctrl,
				      u32 info, void *data)
{
	unsigned int len = FIELD_GET(MT7915_NPU_RX_LEN, ctrl);
	unsigned int cur = FIELD_GET(MT7915_NPU_RX_CUR_LEN, ctrl);
	unsigned int part = FIELD_GET(MT7915_NPU_RX_PART, info);
	unsigned int parts = FIELD_GET(MT7915_NPU_RX_PARTS, info);
	struct sk_buff *skb;

	if (!cur || cur > MT7915_NPU_BUF_SIZE || cur > len)
		goto invalid;

	if (cur == len) {
		skb = napi_alloc_skb(&q->napi, len);
		if (skb)
			skb_put_data(skb, data, len);
		return skb;
	}

	if (part == 1) {
		mt7915_npu_drop_partial(q);
		q->partial = napi_alloc_skb(&q->napi, len);
		q->total = len;
	}

	if (!q->partial || part != q->part + 1U || q->total != len ||
	    cur > len - q->partial->len || cur > skb_tailroom(q->partial))
		goto invalid;

	skb_put_data(q->partial, data, cur);
	q->part = part;
	if (!(ctrl & MT7915_NPU_RX_LAST))
		return NULL;

	if (q->partial->len != len || part != parts)
		goto invalid;

	skb = q->partial;
	q->partial = NULL;
	q->part = 0;
	q->total = 0;
	return skb;

invalid:
	mt7915_npu_drop_partial(q);
	return NULL;
}

static int mt7915_npu_poll(struct napi_struct *napi, int budget)
{
	struct mt7915_npu_queue *q = container_of(napi, typeof(*q), napi);
	struct mt7915_dev *dev = q->dev;
	struct airoha_npu *npu = dev->npu->npu;
	enum mt76_rxq_id qid = MT_RXQ_NPU0 + q->local_band;
	int done = 0;

	rcu_read_lock();
	while (done < budget) {
		struct mt7915_npu_desc *desc = &q->desc[q->head];
		struct sk_buff *skb;
		u32 ctrl = READ_ONCE(desc->ctrl);
		u32 info, vendor;

		if (!(ctrl & MT7915_NPU_RX_DONE))
			break;

		dma_rmb();
		info = READ_ONCE(desc->info);
		vendor = READ_ONCE(desc->vendor);
		skb = mt7915_npu_rx(q, ctrl, info,
				    q->buf + q->head * MT7915_NPU_BUF_SIZE);
		if (skb && !(vendor & MT7915_NPU_RX_PPE) && skb->len >= MT_TXD_SIZE) {
			u32 rx_info = vendor;

			memset(skb->cb, 0, sizeof(skb->cb));
			atomic64_inc(&q->packets);
			mt7915_queue_rx_skb(&dev->mt76, qid, skb, &rx_info);
		} else {
			dev_kfree_skb_any(skb);
		}

		desc->info = 0;
		desc->vendor = 0;
		dma_wmb();
		WRITE_ONCE(desc->ctrl, 0);
		q->head = (q->head + 1) % MT7915_NPU_DESC_NUM;
		atomic64_inc(&q->descriptors);
		done++;
	}

	/* Return descriptors only after all packet bytes have been copied. */
	dma_wmb();
	regmap_write(npu->regmap, q->regs + 0xc, q->head);
	mt76_rx_poll_complete(&dev->mt76, qid, napi);
	if (done < budget && napi_complete_done(napi, done))
		airoha_npu_wlan_enable_irq(npu, q->band);
	rcu_read_unlock();

	return done;
}

static irqreturn_t mt7915_npu_irq(int irq, void *data)
{
	struct mt7915_npu_queue *q = data;
	struct airoha_npu *npu = q->dev->npu->npu;
	u32 status = airoha_npu_wlan_get_irq_status(npu, q->band);
	u32 mask = BIT(16 + q->band);

	if (!(status & mask))
		return IRQ_NONE;

	atomic64_inc(&q->interrupts);
	airoha_npu_wlan_disable_irq(npu, q->band);
	airoha_npu_wlan_set_irq_status(npu, mask);
	napi_schedule_irqoff(&q->napi);
	return IRQ_HANDLED;
}

int mt7915_npu_stop(struct mt7915_dev *dev)
{
	struct mt7915_npu *priv = dev->npu;
	int i, err = 0;

	if (!priv)
		return 0;

	for (i = 0; i < ARRAY_SIZE(priv->q); i++) {
		struct mt7915_npu_queue *q = &priv->q[i];
		int ret;

		if (!(priv->band_mask & BIT(i)))
			continue;
		airoha_npu_wlan_disable_irq(priv->npu, q->band);
		if (q->irq_added)
			synchronize_irq(q->irq);
		if (q->napi_enabled) {
			napi_disable(&q->napi);
			q->napi_enabled = false;
		}
		/* A poll completing concurrently with stop must leave IRQs masked. */
		airoha_npu_wlan_disable_irq(priv->npu, q->band);
		if (!priv->published)
			continue;
		ret = mt7915_npu_send(dev, i, WLAN_FUNC_SET_WAIT_NPU_INIT_DONE,
				      0, GFP_KERNEL);
		if (ret && !err)
			err = ret;
	}
	priv->started = false;
	priv->direct = false;
	if (priv->published)
		priv->retired = true;
	return err;
}

void mt7915_npu_cleanup(struct mt7915_dev *dev)
{
	struct mt7915_npu *priv = dev->npu;
	bool retain;
	int i;

	if (!priv)
		return;

	/* The legacy ABI has no documented DMA-drain acknowledgment. */
	if (mt7915_npu_stop(dev))
		dev_err(dev->mt76.dev, "NPU stop request failed\n");
	retain = priv->published;
	for (i = 0; i < ARRAY_SIZE(priv->q); i++) {
		struct mt7915_npu_queue *q = &priv->q[i];

		if (!(priv->band_mask & BIT(i)))
			continue;
		if (q->irq_added)
			free_irq(q->irq, q);
		if (q->napi_added)
			netif_napi_del(&q->napi);
		mt7915_npu_drop_partial(q);
		if (retain)
			continue;
		if (priv->published) {
			regmap_write(priv->npu->regmap, q->regs + 4, 0);
			regmap_write(priv->npu->regmap, q->regs, 0);
		}
		if (q->desc)
			dma_free_coherent(priv->npu->dev,
					  sizeof(*q->desc) * MT7915_NPU_DESC_NUM,
					  q->desc, q->desc_dma);
		if (q->buf)
			dma_free_coherent(priv->npu->dev,
					  MT7915_NPU_BUF_SIZE * MT7915_NPU_DESC_NUM,
					  q->buf, q->buf_dma);
	}

	/* The transport owns this device, independently of mt76 DMA setup. */
	if (priv->napi_dev)
		free_netdev(priv->napi_dev);
	priv->napi_dev = NULL;

	if (retain) {
		/* A mailbox ACK alone does not prove that all DMA writes are drained. */
		dev_warn(dev->mt76.dev, "experimental NPU RX retains DMA storage and NPU reference until reboot\n");
		dev->npu = NULL;
		return;
	}

	mutex_lock(&mt7915_npu_lock);
	if (priv->ba && !--mt7915_npu_ba_users) {
		dma_free_coherent(priv->npu->dev, MT7915_NPU_BA_SIZE,
				  mt7915_npu_ba_pool, mt7915_npu_ba_dma);
		mt7915_npu_ba_pool = NULL;
	}
	mt7915_npu_queues &= ~priv->queue_mask;
	mutex_unlock(&mt7915_npu_lock);
	airoha_npu_put(priv->npu);
	kfree(priv);
	dev->npu = NULL;
}

int mt7915_npu_init(struct mt7915_dev *dev)
{
	struct device *device = dev->mt76.dev;
	struct mt7915_npu *priv;
	struct airoha_npu *npu;
	struct pci_host_bridge *bridge;
	struct resource res;
	__le32 version;
	enum airoha_npu_wlan_chip model;
	u8 queue;
	int i, j, err;

	bridge = pci_find_host_bridge(to_pci_dev(device)->bus);
	err = of_address_to_resource(bridge->dev.parent->of_node, 0, &res);
	if (err)
		return err;
	if (res.start != 0x1fa91000 && res.start != 0x1fa92000)
		return -EOPNOTSUPP;

	npu = airoha_npu_get(device);
	if (IS_ERR(npu))
		return PTR_ERR(npu);
	if (IS_ENABLED(CONFIG_CPU_BIG_ENDIAN) || !is_mt7915(&dev->mt76) ||
	    !of_device_is_compatible(npu->dev->of_node, "airoha,en7523-npu")) {
		err = -EOPNOTSUPP;
		goto put_npu;
	}
	err = airoha_npu_wlan_get_msg(npu, 0, WLAN_FUNC_GET_WAIT_NPU_VERSION,
				     &version, sizeof(version), GFP_KERNEL);
	if (err)
		goto put_npu;
	if (le32_to_cpu(version) < 0x070e0000 ||
	    le32_to_cpu(version) >> 24 != 7) {
		err = -EOPNOTSUPP;
		goto put_npu;
	}

	model = is_mt7916(&dev->mt76) ? AIROHA_NPU_WLAN_MT7916 :
		    (dev->dbdc_support ? AIROHA_NPU_WLAN_MT7915_DBDC : AIROHA_NPU_WLAN_MT7915);

	priv = kzalloc(sizeof(*priv), GFP_KERNEL);
	if (!priv) {
		err = -ENOMEM;
		goto put_npu;
	}
	/* The vendor mailbox uses P0 = 0 and P1 = 1. */
	priv->port = res.start == 0x1fa92000;
	priv->model = model;
	priv->band_mask = dev->dbdc_support ? GENMASK(1, 0) :
			  BIT(dev->phy.mt76->band_idx);
	mutex_lock(&mt7915_npu_lock);
	queue = ffz(mt7915_npu_queues);
	priv->queue_mask = dev->dbdc_support ? GENMASK(1, 0) : BIT(queue);
	if (queue >= 2 || (mt7915_npu_queues & priv->queue_mask)) {
		mutex_unlock(&mt7915_npu_lock);
		kfree(priv);
		err = -EBUSY;
		goto put_npu;
	}
	mt7915_npu_queues |= priv->queue_mask;
	mutex_unlock(&mt7915_npu_lock);
	for (i = 0; i < ARRAY_SIZE(priv->q); i++) {
		priv->q[i].local_band = i;
		priv->q[i].band = dev->dbdc_support ? i : queue;
	}
	priv->npu = npu;
	dev->npu = priv;

	/* Refuse an already configured host adapter, including a Wi-Fi 7 consumer. */
	for (i = 0; i < ARRAY_SIZE(priv->q); i++) {
		u32 base, count;
		u32 regs;

		if (!(priv->band_mask & BIT(i)))
			continue;
		regs = airoha_npu_wlan_get_queue_addr(npu, priv->q[i].band, false);

		err = regmap_read(npu->regmap, regs, &base);
		if (err)
			goto cleanup;
		err = regmap_read(npu->regmap, regs + 4, &count);
		if (err)
			goto cleanup;
		if (base || count) {
			err = -EBUSY;
			goto cleanup;
		}
	}

	/* mt76 creates its napi_dev later, in mt76_init_queues(). */
	priv->napi_dev = alloc_netdev_dummy(0);
	if (!priv->napi_dev) {
		err = -ENOMEM;
		goto cleanup;
	}

	mutex_lock(&mt7915_npu_lock);
	if (!mt7915_npu_ba_pool)
		mt7915_npu_ba_pool = dma_alloc_coherent(npu->dev, MT7915_NPU_BA_SIZE,
						 &mt7915_npu_ba_dma, GFP_KERNEL);
	priv->ba = mt7915_npu_ba_pool;
	priv->ba_dma = mt7915_npu_ba_dma;
	if (priv->ba)
		mt7915_npu_ba_users++;
	mutex_unlock(&mt7915_npu_lock);
	if (!priv->ba) {
		err = -ENOMEM;
		goto cleanup;
	}
	if (upper_32_bits(priv->ba_dma)) {
		err = -EOVERFLOW;
		goto cleanup;
	}
	for (i = 0; i < ARRAY_SIZE(priv->q); i++) {
		struct mt7915_npu_queue *q = &priv->q[i];

		if (!(priv->band_mask & BIT(i)))
			continue;
		q->dev = dev;
		q->regs = airoha_npu_wlan_get_queue_addr(npu, q->band, false);
		q->irq = npu->irqs[q->band];
		airoha_npu_wlan_disable_irq(npu, q->band);
		q->desc = dma_alloc_coherent(npu->dev,
					     sizeof(*q->desc) * MT7915_NPU_DESC_NUM,
					     &q->desc_dma, GFP_KERNEL);
		q->buf = dma_alloc_coherent(npu->dev,
					    MT7915_NPU_BUF_SIZE * MT7915_NPU_DESC_NUM,
					    &q->buf_dma, GFP_KERNEL);
		if (!q->desc || !q->buf) {
			err = -ENOMEM;
			goto cleanup;
		}
		if (upper_32_bits(q->desc_dma) ||
		    upper_32_bits(q->buf_dma + MT7915_NPU_BUF_SIZE *
				  MT7915_NPU_DESC_NUM - 1)) {
			err = -EOVERFLOW;
			goto cleanup;
		}
		for (j = 0; j < MT7915_NPU_DESC_NUM; j++)
			q->desc[j].addr = lower_32_bits(q->buf_dma + j * MT7915_NPU_BUF_SIZE);

		netif_napi_add(priv->napi_dev, &q->napi, mt7915_npu_poll);
		q->napi_added = true;
		err = request_irq(q->irq, mt7915_npu_irq, IRQF_SHARED,
				  "mt7915-npu-rx", q);
		if (err)
			goto cleanup;
		q->irq_added = true;
	}

	/* Reinitialization also changes band 0's force-to-CPU policy. */
	mutex_lock(&mt7915_npu_lock);
	err = 0;
	if (!mt7915_npu_memory_initialized) {
		err = airoha_npu_wlan_init_reserved_memory(npu);
		if (!err)
			mt7915_npu_memory_initialized = true;
	}
	mutex_unlock(&mt7915_npu_lock);
	if (err)
		goto cleanup;
	err = mt7915_npu_send(dev, __ffs(priv->band_mask),
			      WLAN_FUNC_SET_WAIT_HWNAT_INIT, 1, GFP_KERNEL);
	if (err)
		goto cleanup;
	/* Keep host delivery without a station authorization/replay interface. */
	for (i = 0; i < ARRAY_SIZE(priv->q); i++) {
		if (!(priv->band_mask & BIT(i)))
			continue;
		err = mt7915_npu_send(dev, i, WLAN_FUNC_SET_WAIT_IS_FORCE_TO_CPU,
				      1, GFP_KERNEL);
		if (err)
			goto cleanup;
		err = mt7915_npu_send(dev, i, WLAN_FUNC_SET_WAIT_PCIE_PORT_TYPE,
				      priv->port, GFP_KERNEL);
		if (err)
			goto cleanup;
		err = mt7915_npu_send(dev, i, WLAN_FUNC_SET_WAIT_DRIVER_MODEL,
				      model, GFP_KERNEL);
		if (err)
			goto cleanup;
		err = mt7915_npu_send(dev, i, WLAN_FUNC_SET_WAIT_DRAM_BA_NODE_ADDR,
				      lower_32_bits(priv->ba_dma), GFP_KERNEL);
		if (err)
			goto cleanup;
	}
	dev_info(device, "NPU RX: model %u, port %u, host delivery\n",
			 model, priv->port);
	return 0;

cleanup:
	mt7915_npu_cleanup(dev);
	return err;
put_npu:
	airoha_npu_put(npu);
	return err;
}

int mt7915_npu_rx_ring(struct mt7915_dev *dev, enum mt76_rxq_id qid,
			unsigned int count, u32 regs)
{
	struct mt7915_npu_queue *q;
	unsigned int band = qid == MT_RXQ_BAND1;
	struct pci_dev *pdev = to_pci_dev(dev->mt76.dev);
	phys_addr_t base = pci_resource_start(pdev, 0);
	u32 desc;
	int err;

	if (upper_32_bits(base + regs))
		return -EOVERFLOW;
	q = &dev->npu->q[band];
	if (!(dev->npu->band_mask & BIT(band)))
		return -EINVAL;
	q->radio_regs = regs;
	err = mt7915_npu_send(dev, band, WLAN_FUNC_SET_WAIT_PCIE_ADDR,
			      lower_32_bits(base + regs), GFP_KERNEL);
	if (err)
		return err;
	err = mt7915_npu_send(dev, band, WLAN_FUNC_SET_WAIT_DESC, count, GFP_KERNEL);
	if (err)
		return err;
	err = airoha_npu_wlan_get_msg(dev->npu->npu, q->band,
				     WLAN_FUNC_GET_WAIT_RXDESC_BASE,
				     &desc, sizeof(desc), GFP_KERNEL);
	if (err)
		return err;
	if (!desc || !IS_ALIGNED(desc, 16))
		return -EINVAL;

	q->radio_desc = desc;
	mt76_wr(dev, regs, desc);
	mt76_wr(dev, regs + 4, count);
	mt76_wr(dev, regs + 8, count - 1);
	mt76_wr(dev, regs + 0xc, 0);
	return 0;
}

int mt7915_npu_start(struct mt7915_dev *dev)
{
	struct mt7915_npu *priv = dev->npu;
	int i, err;

	if (!priv)
		return 0;
	if (priv->started)
		return 0;
	if (priv->retired)
		return -ESHUTDOWN;

	for (i = 0; i < ARRAY_SIZE(priv->q); i++) {
		struct mt7915_npu_queue *q = &priv->q[i];

		if (!(priv->band_mask & BIT(i)))
			continue;
		if (!q->radio_desc)
			return -EINVAL;
		q->head = 0;
		mt7915_npu_drop_partial(q);
		for (int j = 0; j < MT7915_NPU_DESC_NUM; j++) {
			q->desc[j].ctrl = 0;
			q->desc[j].info = 0;
			q->desc[j].vendor = 0;
		}
		regmap_write(priv->npu->regmap, q->regs + 4, MT7915_NPU_DESC_NUM);
		regmap_write(priv->npu->regmap, q->regs + 8, 0);
		regmap_write(priv->npu->regmap, q->regs + 0xc, 0);
		dma_wmb();
		regmap_write(priv->npu->regmap, q->regs, lower_32_bits(q->desc_dma));
		priv->published = true;
		napi_enable(&q->napi);
		q->napi_enabled = true;
		err = mt7915_npu_send(dev, i, WLAN_FUNC_SET_WAIT_NPU_INIT_DONE, 1, GFP_KERNEL);
		if (err) {
			mt7915_npu_stop(dev);
			return err;
		}
		airoha_npu_wlan_enable_irq(priv->npu, q->band);
	}
	priv->started = true;
	return 0;
}

int mt7915_npu_ba(struct mt7915_dev *dev, unsigned int band, u16 wcid,
		  u8 tid, u16 ssn, u16 size)
{
	u32 data[] = { wcid, tid, ssn, size };

	if (!dev->npu)
		return 0;
	if (!wcid || wcid > MT7915_NPU_STA_MAX || tid > 7)
		return 0;
	if (size > 256)
		return -EOPNOTSUPP;
	return mt7915_npu_send_data(dev, band, WLAN_FUNC_SET_WAIT_BA_WIN_SIZE,
				    data, sizeof(data));
}

int mt7915_npu_sta_clear(struct mt7915_dev *dev, unsigned int band, u16 wcid)
{
	if (!dev->npu || !wcid || wcid > MT7915_NPU_STA_MAX)
		return 0;

	return mt7915_npu_send(dev, band, WLAN_FUNC_SET_WAIT_DEL_STA,
			      wcid, GFP_KERNEL);
}

void mt7915_npu_rx_status(struct mt7915_dev *dev, struct sk_buff *skb, u32 info)
{
	struct mt76_rx_status *status = (void *)skb->cb;
	struct ieee80211_bar *bar;
	u16 control, ssn;
	u32 val;
	int err;

	/* Only trust reorder and replay checks explicitly reported by Clanker. */
	if (info & MT7915_NPU_RX_PN_VALID)
		status->flag |= RX_FLAG_PN_VALIDATED;
	if (info & MT7915_NPU_RX_REORDERED) {
		status->aggr = false;
		status->flag |= RX_FLAG_DUP_VALIDATED;
	}
	if (info & MT7915_NPU_RX_BYPASS_BA)
		status->aggr = false;
	if (status->flag & RX_FLAG_8023 || skb->len < sizeof(*bar))
		return;
	bar = mt76_skb_get_hdr(skb);
	if (!ieee80211_is_back_req(bar->frame_control) || !status->wcid)
		return;
	control = le16_to_cpu(bar->control);
	if (control & IEEE80211_BAR_CTRL_MULTI_TID ||
	    ((control & IEEE80211_BAR_CTRL_TID_INFO_MASK) >> 12) > 7)
		return;
	ssn = le16_to_cpu(bar->start_seq_num) >> 4;
	val = ((u32)ssn << 11) | ((u32)status->wcid->idx << 3) |
	      ((control & IEEE80211_BAR_CTRL_TID_INFO_MASK) >> 12);
	err = mt7915_npu_send(dev, status->phy_idx, WLAN_FUNC_SET_WAIT_BAR_INFO,
			      val, GFP_ATOMIC);
	if (err)
		dev_warn_ratelimited(dev->mt76.dev, "NPU BAR update failed: %d\n", err);
}

static int mt7915_npu_status_show(struct seq_file *s, void *data)
{
	struct mt7915_dev *dev = s->private;
	struct mt7915_npu *priv = dev->npu;
	int i;

	if (!priv) {
		seq_puts(s, "inactive\n");
		return 0;
	}

	seq_printf(s, "abi: clanker-kite-24\nmodel: %u\nport: %u\nstarted: %u\ndirect: %u\n",
		   priv->model, priv->port, priv->started, priv->direct);
	for (i = 0; i < ARRAY_SIZE(priv->q); i++) {
		struct mt7915_npu_queue *q = &priv->q[i];

		if (!(priv->band_mask & BIT(i)))
			continue;
		seq_printf(s, "band%d: channel=%u irq=%d interrupts=%lld descriptors=%lld packets=%lld\n",
			   i, q->band, q->irq, atomic64_read(&q->interrupts),
			   atomic64_read(&q->descriptors), atomic64_read(&q->packets));
	}
	return 0;
}
DEFINE_SHOW_ATTRIBUTE(mt7915_npu_status);

void mt7915_npu_debugfs(struct mt7915_dev *dev, struct dentry *dir)
{
	if (dev->npu)
		debugfs_create_file("npu_rx", 0400, dir, dev, &mt7915_npu_status_fops);
}
