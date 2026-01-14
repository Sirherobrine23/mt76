#ifndef __MT76_WOE_E_H
#define __MT76_WOE_E_H
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/io.h>
#include <linux/spinlock.h>
#include <linux/skbuff.h>
#include <linux/leds.h>
#include <linux/usb.h>
#include <linux/average.h>
#include <net/mac80211.h>
#include <linux/dma-mapping.h>
#include "util.h"
#include "testmode.h"
#include "mt7915/mt7915.h"
#include "mt7915/mac.h"
#include "dma.h"
#include "mt76.h"
typedef unsigned char UINT8;
typedef unsigned short UINT16;
typedef unsigned int UINT32;
typedef int INT32;
typedef unsigned char	BOOLEAN;
typedef unsigned long	ULONG;
typedef unsigned char	UCHAR;
typedef void 		VOID;
typedef VOID		*PVOID;
typedef int 		INT;
typedef UCHAR * PUCHAR;
#define NDIS_STATUS_SUCCESS                     0x00
#define NDIS_STATUS_FAILURE                     0x01
#define CHIP_ID_WOE 0x7915
#define WHNAT_0 0
#define WHNAT_1 1
typedef unsigned short USHORT;
typedef signed char CHAR;
typedef unsigned char *PUINT8;
#define TRUE 1
#define WED_RX1_CTRL0 0x00000410
#define WED_RX0_CTRL0 0x00000400
//#define MT_INT_WED_MASK_CSR 0xd7204
struct mt76_queue_woe {
	struct mt76_queue_regs __iomem *regs;
	int ndesc;
};
struct woe_need{
	unsigned int ChipID;
	unsigned int tx_res_num;
	BOOLEAN whnat_en;
	//struct hif_pci_tx_ring_desc tx_ring_layout[2];
	struct mt76_queue_woe mt76_queue_info;
	UINT32 int_enable_mask;
	UINT32 token_tx_cnt;
	UINT32 hw_tx_token_cnt;
	unsigned long CSRBaseAddress;
	VOID (*writel_api)(unsigned int value, void *addr,struct mt76_queue *q);
	INT32 (*readl_api)(void *addr, struct mt76_queue *q);
	struct mt76_queue_regs __iomem *regs_phy0 ;
	struct mt76_queue_regs __iomem *regs_phy1 ;
	UINT32 wpdma_base_woe;
	UINT32 wed_int_msk;
	UINT32 irqmask;
	UINT32 intr_return;
	BOOLEAN wed_running;
	UINT32 ring_rx_val;
	UINT32 ring_rx_index;
	UINT32 pAd_index;
	VOID *OS_Cookie;
};
#define TRUE 1
#define WF_WFDMA_EXT_WRAP_CSR_WED_HOST_INT_STA_host_dma0_tx_coherent_int_sts_MASK 0x00800000                // host_dma0_tx_coherent_int_sts[23]
#define WF_WFDMA_EXT_WRAP_CSR_WFDMA_HOST_INT_STA_host_dma0_tx_coherent_int_sts_MASK 0x00800000                // host_dma0_tx_coherent_int_sts[23]
#define WF_WFDMA_EXT_WRAP_CSR_WED_HOST_INT_STA_host_dma1_tx_done_int_sts_16_MASK 0x04000000                // host_dma1_tx_done_int_sts_16[26]
#define WF_WFDMA_EXT_WRAP_CSR_WFDMA_HOST_INT_STA_host_dma1_tx_done_int_sts_16_MASK 0x04000000                // host_dma1_tx_done_int_sts_16[26]
#define WF_WFDMA_EXT_WRAP_CSR_WED_HOST_INT_STA_host_dma1_tx_coherent_int_sts_MASK 0x00200000                // host_dma1_tx_coherent_int_sts[21]
#define WF_WFDMA_EXT_WRAP_CSR_WFDMA_HOST_INT_STA_host_dma1_tx_coherent_int_sts_MASK 0x00200000                // host_dma1_tx_coherent_int_sts[21]
#define WF_WFDMA_EXT_WRAP_CSR_WED_HOST_INT_STA_host_dma0_rx_coherent_int_sts_MASK 0x00400000                // host_dma0_rx_coherent_int_sts[22]
#define WF_WFDMA_EXT_WRAP_CSR_WFDMA_HOST_INT_STA_host_dma0_rx_coherent_int_sts_MASK 0x00400000                // host_dma0_rx_coherent_int_sts[22]
#define WF_WFDMA_EXT_WRAP_CSR_WED_HOST_INT_STA_host_dma1_rx_coherent_int_sts_MASK 0x00100000                // host_dma1_rx_coherent_int_sts[20]
#define WF_WFDMA_EXT_WRAP_CSR_WFDMA_HOST_INT_STA_host_dma1_rx_coherent_int_sts_MASK 0x00100000                // host_dma1_rx_coherent_int_sts[20]
#define WF_WFDMA_EXT_WRAP_CSR_WED_HOST_INT_STA_host_dma1_tx_done_int_sts_17_MASK 0x08000000                // host_dma1_tx_done_int_sts_17[27]
#define WF_WFDMA_EXT_WRAP_CSR_WFDMA_HOST_INT_STA_host_dma1_tx_done_int_sts_17_MASK 0x08000000                // host_dma1_tx_done_int_sts_17[27]
#define WF_WFDMA_EXT_WRAP_CSR_WED_HOST_INT_STA_host_dma1_tx_done_int_sts_20_MASK 0x00008000                // host_dma1_tx_done_int_sts_20[15]
#define WF_WFDMA_EXT_WRAP_CSR_WFDMA_HOST_INT_STA_host_dma1_tx_done_int_sts_20_MASK 0x00008000                // host_dma1_tx_done_int_sts_20[15]
#define WF_WFDMA_EXT_WRAP_CSR_WED_HOST_INT_STA_host_dma0_rx_done_int_sts_0_MASK 0x00010000                // host_dma0_rx_done_int_sts_0[16]
#define WF_WFDMA_EXT_WRAP_CSR_WFDMA_HOST_INT_STA_host_dma0_rx_don_int_sts_0_MASK 0x00010000                // host_dma0_rx_don_int_sts_0[16]
#define WF_WFDMA_EXT_WRAP_CSR_WED_HOST_INT_STA_host_dma0_rx_done_int_sts_1_MASK 0x00020000                // host_dma0_rx_done_int_sts_1[17]
#define WF_WFDMA_EXT_WRAP_CSR_WFDMA_HOST_INT_STA_host_dma0_rx_don_int_sts_1_MASK 0x00020000                // host_dma0_rx_don_int_sts_1[17]
#define WF_WFDMA_EXT_WRAP_CSR_WED_HOST_INT_STA_host_dma1_rx_done_int_sts_0_MASK 0x00000001                // host_dma1_rx_done_int_sts_0[0]
#define WF_WFDMA_EXT_WRAP_CSR_WFDMA_HOST_INT_STA_host_dma1_rx_done_int_sts_0_MASK 0x00000001                // host_dma1_rx_done_int_sts_0[0]
#define WF_WFDMA_EXT_WRAP_CSR_WED_HOST_INT_STA_host_dma1_rx_done_int_sts_1_MASK 0x00000002                // host_dma1_rx_done_int_sts_1[1]
#define WF_WFDMA_EXT_WRAP_CSR_WFDMA_HOST_INT_STA_host_dma1_rx_done_int_sts_1_MASK 0x00000002                // host_dma1_rx_done_int_sts_1[1]
#define WF_WFDMA_EXT_WRAP_CSR_WED_HOST_INT_STA_host_dma1_rx_done_int_sts_2_MASK 0x00000004                // host_dma1_rx_done_int_sts_2[2]
#define WF_WFDMA_EXT_WRAP_CSR_WFDMA_HOST_INT_STA_host_dma1_rx_done_int_sts_2_MASK 0x00000004                // host_dma1_rx_done_int_sts_2[2]
#define WF_WFDMA_EXT_WRAP_CSR_WED_HOST_INT_STA_host_dma1_mcu2host_sw_int_sts_MASK 0x20000000                // host_dma1_mcu2host_sw_int_sts[29]
#define WF_WFDMA_EXT_WRAP_CSR_WFDMA_HOST_INT_STA_host_dma1_mcu2host_sw_int_sts_MASK 0x20000000                // host_dma1_mcu2host_sw_int_sts[29]
#define WF_WFDMA_EXT_WRAP_CSR_WED_HOST_INT_STA_host_dma1_subsys_int_sts_MASK 0x10000000                // host_dma1_subsys_int_sts[28]
#define WF_WFDMA_EXT_WRAP_CSR_WFDMA_HOST_INT_STA_host_dma1_subsys_int_sts_MASK 0x10000000                // host_dma1_subsys_int_sts[28]

#define MT_INT_DMA0_TX_COHE		WF_WFDMA_EXT_WRAP_CSR_WED_HOST_INT_STA_host_dma0_tx_coherent_int_sts_MASK
#define MT_INT_DMA1_TX_COHE		WF_WFDMA_EXT_WRAP_CSR_WED_HOST_INT_STA_host_dma1_tx_coherent_int_sts_MASK
#define MT_INT_DMA0_RX_COHE		WF_WFDMA_EXT_WRAP_CSR_WED_HOST_INT_STA_host_dma0_rx_coherent_int_sts_MASK
#define MT_INT_DMA1_RX_COHE		WF_WFDMA_EXT_WRAP_CSR_WED_HOST_INT_STA_host_dma1_rx_coherent_int_sts_MASK

#define MT_INT_DMA1_T16_DONE	WF_WFDMA_EXT_WRAP_CSR_WED_HOST_INT_STA_host_dma1_tx_done_int_sts_16_MASK
#define MT_INT_DMA1_T17_DONE	WF_WFDMA_EXT_WRAP_CSR_WED_HOST_INT_STA_host_dma1_tx_done_int_sts_17_MASK
#define MT_INT_DMA1_T20_DONE	WF_WFDMA_EXT_WRAP_CSR_WED_HOST_INT_STA_host_dma1_tx_done_int_sts_20_MASK
#define MT_INT_DMA0_R0_DONE		WF_WFDMA_EXT_WRAP_CSR_WED_HOST_INT_STA_host_dma0_rx_done_int_sts_0_MASK
#define MT_INT_DMA0_R1_DONE		WF_WFDMA_EXT_WRAP_CSR_WED_HOST_INT_STA_host_dma0_rx_done_int_sts_1_MASK
#define MT_INT_DMA1_R0_DONE		WF_WFDMA_EXT_WRAP_CSR_WED_HOST_INT_STA_host_dma1_rx_done_int_sts_0_MASK
#define MT_INT_DMA1_R1_DONE		WF_WFDMA_EXT_WRAP_CSR_WED_HOST_INT_STA_host_dma1_rx_done_int_sts_1_MASK
#define MT_INT_DMA1_R2_DONE		WF_WFDMA_EXT_WRAP_CSR_WED_HOST_INT_STA_host_dma1_rx_done_int_sts_2_MASK
#define MT_INT_MCU2HOST_SW_INT_STS		WF_WFDMA_EXT_WRAP_CSR_WED_HOST_INT_STA_host_dma1_mcu2host_sw_int_sts_MASK
#define MT_INT_SUBSYS_INT_STS			WF_WFDMA_EXT_WRAP_CSR_WED_HOST_INT_STA_host_dma1_subsys_int_sts_MASK

#define MT_INT_TxCoherent		(MT_INT_DMA0_TX_COHE | MT_INT_DMA1_TX_COHE)
#define MT_INT_RxCoherent		(MT_INT_DMA0_RX_COHE | MT_INT_DMA1_RX_COHE)
#define MT_INT_CoherentInt		(MT_INT_TxCoherent | MT_INT_RxCoherent)

#define MT_INT_RX_DATA		(MT_INT_DMA0_R0_DONE | MT_INT_DMA0_R1_DONE)
#define MT_INT_RX_CMD		(MT_INT_DMA1_R0_DONE | MT_INT_DMA1_R1_DONE | MT_INT_DMA1_R2_DONE)
#define MT_INT_RX_W			(MT_INT_RX_DATA | MT_INT_RX_CMD)

#define TX_RES_NUM 2
#define MT7915_TX_RING_SIZE		2048
#define MT7915_HW_TOKEN_SIZE_WOE 7168
#define WED_REG_BASE 0
#define  WED_TX0_CTRL0 (WED_REG_BASE + 0x00000300)
#define  WED_TX0_CTRL1 (WED_REG_BASE + 0x00000304)
#define  WED_TX0_CTRL2 (WED_REG_BASE + 0x00000308)
#define  WED_TX0_CTRL3 (WED_REG_BASE + 0x0000030C)
#define  WED_TX1_CTRL0 (WED_REG_BASE + 0x00000310)
#define  WED_TX1_CTRL1 (WED_REG_BASE + 0x00000314)
#define  WED_TX1_CTRL2 (WED_REG_BASE + 0x00000318)
#define  WED_TX1_CTRL3 (WED_REG_BASE + 0x0000031C)
extern unsigned int wed_base_addr;
extern unsigned int wed_base_addr_1;
extern int woe_enable(void);
extern void set_woe_enable(int enable);
extern int mt_wlan_hook_call(unsigned short hook, void *ad, void *priv);
typedef enum {
	WLAN_HOOK_FIRST = 0,
	WLAN_HOOK_HIF_INIT = 0,
	WLAN_HOOK_HIF_EXIT,
	//WLAN_HOOK_PEER_LINKUP,
	//WLAN_HOOK_PEER_LINKDOWN,
	WLAN_HOOK_TX,
	//WLAN_HOOK_RX,
	WLAN_HOOK_SYS_UP,
	WLAN_HOOK_SYS_DOWN,
	WLAN_HOOK_ISR,
	WLAN_HOOK_DMA_SET,
	WLAN_HOOK_SER,
	//WLAN_HOOK_SUSPEND,
	WLAN_HOOK_RESUME,
	WLAN_HOOK_UPDATE,
	WLAN_HOOK_IRQ_GET,
	WLAN_HOOK_RING_RX_UP,
	WLAN_HOOK_WOE_STOP,
	WLAN_HOOK_WOE_WDMA_RESET,
	WLAN_HOOK_RING_RESET,	
	WLAN_HOOK_END
} WLAN_HOOK_PT;
extern void writel_mt76(unsigned int value, void *addr,struct mt76_queue *q);
extern void writel_mt76(unsigned int value, void *addr,struct mt76_queue *q);

typedef struct pci_dev		*PPCI_DEV;
typedef struct _OS_RSTRUC  {
	UCHAR *pContent; /* pointer to real structure content */
} OS_RSTRUC;

typedef OS_RSTRUC RTMP_NET_TASK_STRUCT;
#define RTMP_OS_PID						ULONG /* value or pointer */
typedef enum _RTMP_INF_TYPE_ {
	RTMP_DEV_INF_UNKNOWN = 0,
	RTMP_DEV_INF_PCI = 1,
	RTMP_DEV_INF_USB = 2,
	RTMP_DEV_INF_RBUS = 4,
	RTMP_DEV_INF_PCIE = 5,
	RTMP_DEV_INF_SDIO = 6,
} RTMP_INF_TYPE;
#define ra_dma_addr_t					unsigned long long
typedef	ra_dma_addr_t			NDIS_PHYSICAL_ADDRESS;
typedef void				*PNDIS_PACKET;
typedef struct _RTMP_DMABUF {
	ULONG AllocSize;
	PVOID AllocVa;
	NDIS_PHYSICAL_ADDRESS AllocPa;
} RTMP_DMABUF, *PRTMP_DMABUF;
struct os_cookie
{
//#ifdef RTMP_MAC_PCI
	PPCI_DEV pci_dev;
	PPCI_DEV parent_pci_dev;
	USHORT DeviceID;
//#endif /* RTMP_MAC_PCI */
	struct device *pDev;
	UINT32 pAd_va;
#if defined(RTMP_MAC_PCI) || defined(RTMP_MAC_USB)
#ifdef UAPSD_SUPPORT
	RTMP_NET_TASK_STRUCT uapsd_eosp_sent_task;
#endif /* UAPSD_SUPPORT */
#ifdef CONFIG_AP_SUPPORT
#ifdef CARRIER_DETECTION_SUPPORT
	RTMP_NET_TASK_STRUCT carrier_sense_task;
#endif /* CARRIER_DETECTION_SUPPORT */
#endif /* CONFIG_AP_SUPPORT */
#endif
	RTMP_OS_PID			apd_pid; /*802.1x daemon pid */
	unsigned long			apd_pid_nr;
#ifdef CONFIG_AP_SUPPORT
#ifdef IAPP_SUPPORT
	/*	RT_SIGNAL_STRUC			RTSignal; */
	RTMP_OS_PID			IappPid; /*IAPP daemon pid */
	unsigned long			IappPid_nr;
#endif /* IAPP_SUPPORT */
#endif /* CONFIG_AP_SUPPORT */
	INT						ioctl_if_type;
	INT					ioctl_if;
	struct _SECURITY_CONFIG *pSecConfig;
#ifdef FW_DUMP_SUPPORT
	struct proc_dir_entry *proc_fwdump_dir;
	struct proc_dir_entry *proc_fwdump_file;
	CHAR fwdump_dir_name[11];
#endif
};
typedef struct os_cookie	*POS_COOKIE;
typedef struct _RTMP_DMACB {
	ULONG AllocSize;	/* Control block size */
	PVOID AllocVa;		/* Control block virtual address */
	NDIS_PHYSICAL_ADDRESS AllocPa;	/* Control block physical address */
	PNDIS_PACKET pNdisPacket;
	PNDIS_PACKET pNextNdisPacket;
	NDIS_PHYSICAL_ADDRESS PacketPa;
	RTMP_DMABUF DmaBuf;
} RTMP_DMACB, *PRTMP_DMACB ____cacheline_aligned;
typedef spinlock_t			OS_NDIS_SPIN_LOCK;
#define NDIS_SPIN_LOCK							OS_NDIS_SPIN_LOCK
typedef unsigned char UINT8;
typedef unsigned short UINT16;

#ifdef DBDC_MODE
#define DBDC_BAND_NUM		2
#else
#define DBDC_BAND_NUM		1
#endif /* DBDC_MODE */
enum PACKET_TYPE {
	TX_DATA,
	TX_DATA_HIGH_PRIO,
	TX_MGMT,
	TX_ALTX,
	TX_CMD,
	TX_FW_DL,
	TX_DATA_PS,
	RX_PPE_VALID,
	PACKET_TYPE_NUM,
};
#define WMM_QUE_NUM 4
enum buf_alloc_type {
	DYNAMIC_PAGE_ALLOC,
	DYNAMIC_SLAB_ALLOC,
	PRE_SLAB_ALLOC,
	DYNAMIC_PAGE_ALLOC_DEBUG,
	PKT_ALLOC_TYPE_NUMS
};
enum {
	GET_PKT_DDONE,
	GET_PKT_IO,
	GET_PKT_METHOD_NUMS
};
#if 0
#define MT_WFDMA_HOST_CONFIG_WED	BIT(1)
#define MT_QFLAG_WED_RING	GENMASK(1, 0)
#define MT_QFLAG_WED_TYPE	GENMASK(3, 2)
#define MT_QFLAG_WED		BIT(4)
#define __MT_WED_Q(_type, _n)	(MT_QFLAG_WED | \
				 FIELD_PREP(MT_QFLAG_WED_TYPE, _type) | \
				 FIELD_PREP(MT_QFLAG_WED_RING, _n))

#define MT_WED_Q_TX(_n)		__MT_WED_Q(MT76_WED_Q_TX, _n)
#define MT_WED_Q_TXFREE		__MT_WED_Q(MT76_WED_Q_TXFREE, 0)
enum mt76_wed_type {
	MT76_WED_Q_TX,
	MT76_WED_Q_TXFREE,
};
#endif
#define TX_QUEUE0_INDEX 18
#define TX_QUEUE1_INDEX 19
#define MT_RXQ_MCU_WA_RING_BASE 0xd7400
extern struct woe_need pAd;
extern struct woe_need pAd_1;
struct wlan_tx_info_woe {
	unsigned char *pkt;
	unsigned int bssidx;
	unsigned int wcid;
	unsigned int ringidx;
};

static inline void
mtk_wed_irq_set_mask_woe(struct mt76_dev *dev,unsigned int mask)
{
#ifdef CONFIG_SOC_WOE
	if(dev->slot_id ==  0)
	{
		if(!(pAd.wed_running))
			return;
		pAd.wed_int_msk = mask;
		mt_wlan_hook_call(WLAN_HOOK_UPDATE, &pAd, NULL);		
	}
	else
#endif
	{
		if(!(pAd_1.wed_running))
			return;
		pAd_1.wed_int_msk = mask;
		mt_wlan_hook_call(WLAN_HOOK_UPDATE, &pAd_1, NULL);		
	}
}

static inline INT os_alloc_mem(
	VOID *pAd,
	UCHAR **mem,
	ULONG size)
{
	*mem = (PUCHAR) kmalloc(size, GFP_ATOMIC);

	if (*mem) {
		return NDIS_STATUS_SUCCESS;
	} else
		return NDIS_STATUS_FAILURE;
}


static inline void woe_start(struct pci_dev *pdev, struct mt76_dev *mdev, unsigned long csr_addr, u32 wpdma_base_woe)
{
	struct os_cookie *handle = NULL;
	struct woe_need pAd_woe;
#ifdef CONFIG_SOC_WOE
	if (pdev->bus)
		mdev->slot_id = (pdev->bus->self->devfn >> 3) & 0x1f;
	else
		mdev->slot_id = 1;
	if(mdev->slot_id == 0)
		pAd_woe.pAd_index = WHNAT_0;
	else
#endif
		pAd_woe.pAd_index = WHNAT_1;
	/*modify to get from pcie init*/
	pAd_woe.ChipID = CHIP_ID_WOE;
	/*modify get from wifi config*/
	pAd_woe.whnat_en = TRUE;
	/*get slot id*/
	os_alloc_mem(NULL, (UCHAR **)&handle, sizeof(struct os_cookie));
	if (handle == NULL) {
		printk("%s:%d handle alloc fail !!!\n",__func__,__LINE__);
	}
	memset(handle, 0, sizeof(struct os_cookie));
	((POS_COOKIE)handle)->pci_dev = pdev;
	((POS_COOKIE)handle)->pDev = &(pdev->dev);
	pAd_woe.OS_Cookie = handle;
	
	/*tx res num & tx ring size*/
	pAd_woe.tx_res_num = TX_RES_NUM;
	pAd_woe.mt76_queue_info.ndesc = MT7915_TX_RING_SIZE;
	/*int_enable_mask config wifi chip*/
	pAd_woe.int_enable_mask = MT_INT_CoherentInt | MT_INT_DMA1_T16_DONE |
							MT_INT_DMA1_T17_DONE | MT_INT_DMA1_T20_DONE | MT_INT_RX_W;
	pAd_woe.int_enable_mask |= MT_INT_MCU2HOST_SW_INT_STS | MT_INT_SUBSYS_INT_STS;
	/*token number */
	pAd_woe.token_tx_cnt = MT7915_TOKEN_SIZE;
	pAd_woe.hw_tx_token_cnt = MT7915_HW_TOKEN_SIZE_WOE;
	pAd_woe.CSRBaseAddress = csr_addr;
	//MT_WED_TX_RING_BASE+dev->mmio.regs
	pAd_woe.wpdma_base_woe = wpdma_base_woe;
	if(pAd_woe.pAd_index == WHNAT_0)
	{
		memcpy(&pAd, &pAd_woe, sizeof(pAd_woe));
		mt_wlan_hook_call(WLAN_HOOK_SYS_UP, &pAd, NULL);	
	}
	else
	{
		memcpy(&pAd_1, &pAd_woe, sizeof(pAd_woe));
		mt_wlan_hook_call(WLAN_HOOK_SYS_UP, &pAd_1, NULL);	
	}	
}

 static inline void mtk_wed_irq_set_mask(struct mt7915_dev *dev, unsigned int mask)
 {
#ifdef CONFIG_SOC_WOE
	if(dev->mt76.slot_id == 0)
	{
		if(!(pAd.wed_running))
			return;
		pAd.wed_int_msk = mask;
		mt_wlan_hook_call(WLAN_HOOK_UPDATE, &pAd, NULL);
	}
	else
#endif
	{
		if(!(pAd_1.wed_running))
			return;
 		pAd_1.wed_int_msk = mask;
		mt_wlan_hook_call(WLAN_HOOK_UPDATE, &pAd_1, NULL);
	}	
 }

static inline u32
mtk_wed_irq_get(struct mt7915_dev *dev, u32 mask)
{
#ifdef CONFIG_SOC_WOE
	if(dev->mt76.slot_id == 0)
	{
   		pAd.irqmask = mask;
   		mt_wlan_hook_call(WLAN_HOOK_IRQ_GET, &pAd, NULL);
   		return pAd.intr_return;
	}
	else
#endif
	{
   		pAd_1.irqmask = mask;
   		mt_wlan_hook_call(WLAN_HOOK_IRQ_GET, &pAd_1, NULL);
   		return pAd_1.intr_return;
	}		
}

static inline void
mt76_dma_sync_idx_woe(struct mt76_dev *dev, struct mt76_queue *q)
{
#ifdef CONFIG_SOC_WOE
	if(dev->slot_id == 0)
	{
		pAd.writel_api(q->desc_dma, &q->regs->desc_base, q);
		pAd.writel_api(q->ndesc, &q->regs->ring_size, q);
		q->head = pAd.readl_api(&q->regs->dma_idx, q);
		q->tail = q->head;
	}
	else
#endif
	{
		pAd_1.writel_api(q->desc_dma, &q->regs->desc_base, q);
		pAd_1.writel_api(q->ndesc, &q->regs->ring_size, q);
		q->head = pAd_1.readl_api(&q->regs->dma_idx, q);
		q->tail = q->head;		
	}
}
static inline void
mt76_dma_queue_reset_woe(struct mt76_dev *dev, struct mt76_queue *q)
{
	int i;
	if (!q || !q->ndesc)
		return;
	/* clear descriptors */
	for (i = 0; i < q->ndesc; i++)
		q->desc[i].ctrl = cpu_to_le32(MT_DMA_CTL_DMA_DONE);
#ifdef CONFIG_SOC_WOE
	if(dev->slot_id == 0)
	{
		pAd.writel_api(0, &q->regs->cpu_idx, q);
		pAd.writel_api(0, &q->regs->dma_idx, q);
	}
	else
#endif
	{
		pAd_1.writel_api(0, &q->regs->cpu_idx, q);
		pAd_1.writel_api(0, &q->regs->dma_idx, q);
	}
	mt76_dma_sync_idx_woe(dev, q);
}
//int mt76_dma_rx_fill(struct mt76_dev *dev, struct mt76_queue *q);
static inline int
mtk_wed_device_txfree_ring_setup(struct mt76_dev *dev, void __iomem *regs)
{
   int i;
   u32 val;
   for (i = 0; i < 12; i += 4) {
	   val = readl(regs + i);
#ifdef CONFIG_SOC_WOE
	   if(dev->slot_id == 0)
	   {
	   		pAd.ring_rx_val = val;
	   		pAd.ring_rx_index = i;
	   		mt_wlan_hook_call(WLAN_HOOK_RING_RX_UP, &pAd, NULL);
	   }
	   else
#endif
	   {
	   		pAd_1.ring_rx_val = val;
	   		pAd_1.ring_rx_index = i;
	   		mt_wlan_hook_call(WLAN_HOOK_RING_RX_UP, &pAd_1, NULL);
	   }	   	
   }
   return 0;
}

#if 0
static inline int
mt76_dma_wed_setup(struct mt76_dev *dev, struct mt76_queue *q, bool reset)
{
	//struct mtk_wed_device *wed = &dev->mmio.wed;
	int ret, type, ring, i;
	u8 flags = q->flags;
	type = FIELD_GET(MT_QFLAG_WED_TYPE, q->flags);
	ring = FIELD_GET(MT_QFLAG_WED_RING, q->flags);
	switch (type) {
	case MT76_WED_Q_TXFREE:
		/* WED txfree queue needs ring to be initialized before setup */
		q->flags = 0;
		for (i = 0; i < q->ndesc; i++)
			q->desc[i].ctrl = cpu_to_le32(MT_DMA_CTL_DMA_DONE);
		writel(0, &q->regs->cpu_idx);
		writel(0, &q->regs->dma_idx);
		writel(q->desc_dma, &q->regs->desc_base);
		writel(q->ndesc, &q->regs->ring_size);
		q->head = readl(&q->regs->dma_idx);
		q->tail = q->head;	
		mt76_dma_rx_fill(dev, q);
		q->flags = flags;
		q->wed_regs = WED_RX1_CTRL0;
		ret = mtk_wed_device_txfree_ring_setup(dev, q->regs);
		break;
	default:
		ret = 0;
	}

	return ret;
}
#endif
static inline int woe_read_1(struct mt76_dev *dev, struct mt76_queue *q)
{
	int val;
	if(q->flags == 1)
	{
#ifdef CONFIG_SOC_WOE
		if(dev->slot_id == 0)
			val = readl((void *)(wed_base_addr + q->wed_regs + 12));
		else
#endif
			val = readl((void *)(wed_base_addr_1 + q->wed_regs + 12));
	}
	else
		val = readl(&q->regs->dma_idx);
	return val;
}
#endif