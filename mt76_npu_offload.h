#ifndef NPU_OFFLOAD_H
#define NPU_OFFLOAD_H
#ifdef TCSUPPORT_NPU_WIFI_OFFLOAD	

#include "modules/npu/wifi_mail.h"
#include "mt7915/mt7915.h"
#include "dma.h"
#include "mt7915/mac.h"
#include "mt76.h"

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pci.h>

#ifdef CONFIG_SOC_WOE 
#include "mt76_woe.h"
#endif


#define BAND5G_IDX			1
#define BAND2G_IDX			0


#define HOST_COUNTER_SIZE 5
#define NPU_MAX_TYPE 3





#define HOST_COUNTER(i)		(*((unsigned long int *)((unsigned long int)HCounter_Base + (i << 2))))
#define NPU_COUNTER(addr,i)	(*((unsigned long int *)((unsigned long int)addr + (i << 2))))
#define HOST_COUNTER_ADDR(i)	((unsigned long int)HCounter_Base + (i << 2))
#define NPU_COUNTER_ADDR(addr,i)	((unsigned long int)addr + (i << 2))

enum NPU_OFFLOAD_COUNTER_OFFSET
{
        HOST_WHILE_COUNT = 0,           //00
        RESCHEDULE_COUNT = 1,           //01
        PKT_NULL_COUNT,                 //02
        HOST_GET_2GPKT_COUNT,           //03
        HOST_GET_5GPKT_COUNT,           //04
	HCOUNTER_BOTTOM			//
};

typedef enum {
	P0_ = 0,
	P1_,
	P0__P0,
	P0__P1
} PortType_t;

enum COUNTER_OFFSET{
	GET_BUFID_FAIL = 0,		//00v
	WHILE_COUNT = 1,		//01v
	ALL_GET_PKT_COUNT,		//03v
	DROP_PACKETS,			//04v
	//ENQ_BUFID_FIAL_COUNT,		//05v
	//ALL_ENQ_PKT_COUNT,		//06v
	ENQ_DEQ_WHILE_COUNT,		//07v
	ENQ_SRAM_COUNT,			//08v
	ENQ_SRAM_FULL_COUNT,		//09vv
	ENQ_SRAM_FAIL_COUNT,		//09v
	DEQ_SRAM_COUNT,			//11v
	DEQ_SRAM_FAIL_BUFID_COUNT,	//12v
	DEQ_SRAM_FAIL_LEN_COUNT,	//13v
	//DEQ_SRAM_NO_INFO_COUNT,		//14v
	TO_HOSTAPD_COUNT,			//15vv
	//ENQ_DRAM_FAIL_COUNT,		//16v
	//ENQ_BUFID_ERROR_COUNT,		//17v
	BA_IN_ORDER_PKT_COUNT,		//18v
	BA_DUPL_PKT_COUNT,		//19v
	BA_OLD_PKT_COUNT,		//20v
	BA_WITHIN_WS_PKT_COUNT,		//21v
	BA_POP_PKT_COUNT,		//22v
	//BA_NO_PKT_IN_LIST_COUNT,	//23v
	BA_AMSDU_COUNT,			//24v
	AMPDU_COUNT,			//25v
	//BA_NOT_DATA_PKT_COUNT,		//26v
	//BA_RETURN_ERROR_COUNT,		//27v
	BA_NO_MEM_COUNT,		//28v
	TO_QDMA_COUNT,			//29v
	TO_QDMA_COUNT_88,
	TO_QDMA_COUNT_512,
	TO_QDMA_COUNT_1518,
	//ACCESS_PACKET,			//30v
	HOST_APD_ERROR_COUNT,		//31v
	//QDMA_RETURN_UNBIND_COUNT,	//32v
	QDMA_RETURN_FAIL_COUNT,		//33v
	//QDMA_RETURN_OK_COUNT,		//34v
	SCATTER_CNT_MORE1,		//35v
	BIGGER_PACKET,			//36v
	//BA_NODE_ALLOC_FAIL,	 	//37
	//BA_NODE_ALLOC_COUNT,		//38
	BA_AMSDU_MISS,			//39
	//BA_TIMEOUT_FLUSH,		//40
	RX_DESC_DDONE,			//41
	BA_TIMEOUT_FLUSH100,		//42
	BA_TIMEOUT_FLUSH250,		//43
	BA_ENQ_DUP_SEQ,			//vv
	BA_REORDERING_NODE_FREE_COUNT,	//10vv
	BA_REORDERINT_NODE_ALLOC_FAIL, 	//11vv
	BA_REORDERING_NODE_ALLOC_COUNT,	//12vv
	BA_REORDERING_NODE_ALLOC_DRAM_COUNT,//vv
	COUNTER_BOTTOM			//47
};
#define NPU_COUNTER_SIZE		COUNTER_BOTTOM
#define UTIL_COUNTER_NUM		UCOUNTER_BOTTOM

enum COUNTER_OFFSET_UTIL{
	QDMA_TXDESC_NULL_COUNT=0,	//0vv
	QDMA_FREE_BUFID_COUNT,		//1vv
	QDMA_UNBIND_COUNT,		//2vv
	QDMA_TO_ENQ_FAIL_COUNT,		//3vv
	QDMA_TO_ENQ_FAIL_COUNT2,	//4
	//QDMA_TO_PPE_COUNT,		//5
	QDMA_DONE_COUNT,		//6vv
	SKB_FREE_BUFID_COUNT,		//7vv
	SKB_ALLOC_BUFID_COUNT,		//8vv
	SKB_BUFID_ALLOC_FAIL,		//9vv
	QDMA_TX_DSCP_IDX_INVALID,	//13vv
	QDMA_TX_DSCP_INFO_ERROR,	//14vv
	QDMA_DONE_DROP_BIT_ERROR,		//15
	//QDMA_TXDESC_PUSH_COUNT,		//16
	//QDMA_TXDESC_PUSH2_COUNT,	//17
	//QDMA_TXDESC_POP_COUNT,		//18
	UCOUNTER_BOTTOM			//19
};


#define ra_dma_addr_t					unsigned long long
typedef	ra_dma_addr_t			NDIS_PHYSICAL_ADDRESS;
typedef void				*PNDIS_PACKET;

void wifi_offload_init_npu(struct pci_dev *pdev,struct mt7915_dev *dev,int flag);
void npu_init_rxD(struct mt7915_dev *dev,struct pci_dev *pdev);


void get_npu_rxd(struct mt7915_dev *dev,struct pci_dev *pdev);
int npu_enable(void);
void set_npu_enable(int enable);

int mt7915_npu_count(struct seq_file *s, void *data);




#endif
#endif
