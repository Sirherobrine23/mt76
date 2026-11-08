// SPDX-License-Identifier: BSD-3-Clause-Clear
#include <linux/pci.h>

#include "mt7615.h"
#include "mac.h"

#if IS_ENABLED(CONFIG_NET_AIROHA_SOC_WED)
static bool wed_enable = true;
module_param(wed_enable, bool, 0644);
MODULE_PARM_DESC(wed_enable, "Enable Airoha WED transmit offload");

static u32 mt7615_wed_init_buf(void *ptr, dma_addr_t phys, int token)
{
	struct mt76_connac_fw_txp *txp = ptr + MT_TXD_SIZE;
	__le32 *txwi = ptr;
	u32 size = MT_TXD_SIZE + sizeof(*txp);

	memset(ptr, 0, size);
	txwi[0] = cpu_to_le32(FIELD_PREP(MT_TXD0_TX_BYTES, MT_TXD_SIZE));
	txwi[1] = cpu_to_le32(MT_TXD1_LONG_FORMAT |
			     FIELD_PREP(MT_TXD1_PKT_FMT, MT_TX_TYPE_CT) |
			     FIELD_PREP(MT_TXD1_HDR_FORMAT, MT_HDR_FORMAT_802_3) |
			     FIELD_PREP(MT_TXD1_HDR_PAD, 1));
	txp->token = cpu_to_le16(token);
	txp->nbuf = 1;
	txp->buf[0] = cpu_to_le32(phys + size);

	return size;
}
#endif

int mt7615_wed_init(struct mt7615_dev *dev, struct device *device, int *irq)
{
#if IS_ENABLED(CONFIG_NET_AIROHA_SOC_WED)
	struct mtk_wed_device *wed = &dev->mt76.mmio.wed;
	struct pci_dev *pdev;
	u32 base;

	if (!wed_enable || !is_mt7615(&dev->mt76) || !dev_is_pci(device))
		return 0;

	pdev = to_pci_dev(device);
	base = pci_resource_start(pdev, 0);
	wed->wlan.pci_dev = pdev;
	wed->wlan.bus_type = MTK_WED_BUS_PCIE;
	wed->wlan.base = dev->mt76.mmio.regs;
	wed->wlan.phy_base = base;
	wed->wlan.wpdma_phys = base + dev->reg_map[MT_HIF_BASE];
	wed->wlan.wpdma_int = base + MT_INT_SOURCE_CSR;
	wed->wlan.wpdma_mask = base + MT_INT_MASK_CSR;
	wed->wlan.wpdma_tx = base + MT_TX_RING_BASE;
	wed->wlan.wpdma_txfree = base + MT_RX_RING_BASE + 0x10;
	wed->wlan.wpdma_rx_glo = base + MT_WPDMA_GLO_CFG;
	wed->wlan.rx_tbit[0] = 0;
	wed->wlan.tx_tbit[0] = 4;
	wed->wlan.tx_tbit[1] = 5;
	wed->wlan.txfree_tbit = 1;
	wed->wlan.nbuf = 2048;
	wed->wlan.token_start = MT7615_TOKEN_SIZE - wed->wlan.nbuf;
	wed->wlan.init_buf = mt7615_wed_init_buf;
	wed->wlan.offload_enable = mt76_wed_offload_enable;
	wed->wlan.offload_disable = mt76_wed_offload_disable;

	if (mtk_wed_device_attach(wed))
		return 0;

	dev->mt76.dma_dev = wed->dev;
	*irq = wed->irq;
#endif
	return 0;
}
