#ifdef TCSUPPORT_NPU_WIFI_OFFLOAD
#include "mt76_npu_offload.h"
extern unsigned long int  *dramReorderNodeBase;
/*
extern int setonce;

extern unsigned long int *HCounter_Base;
extern unsigned long int *NCounter_Base;
extern unsigned long int *NCounter2G_Base;
extern unsigned long int *NCounterU_Base;

*/

unsigned long int *HCounter_Base;//, *dbg_flag, *dbg2_flag;
unsigned long int *NCounter_Base;

unsigned long int  *NCounter2G_Base, *NCounterU_Base;//, *dbg_flag, *dbg2_flag;
unsigned long int  *dramReorderNodeBase;
int setonce=0;


int npu_enable_chk = 1;
extern int dev_dbdc;
void npu_init_rxD(struct mt7915_dev *dev,struct pci_dev *pdev)
{
	int isMailBoxSuccess=0;
	int ring_size=1536;
	unsigned char band_idx = 0;
	if (pdev->bus)
		band_idx = (pdev->bus->self->devfn >> 3) & 0x1f;
	if(dev_dbdc)
		band_idx = 0;
	isMailBoxSuccess = WIFI_MAIL_API_SET_WAIT_DESC(band_idx, ring_size);
	if(isMailBoxSuccess != 1)
		printk("Error: NPU_WIFI_OFFLOAD Set RXD ring size fail\n");
	if(dev_dbdc)
	{
		isMailBoxSuccess = WIFI_MAIL_API_SET_WAIT_DESC(1, ring_size);
		if(isMailBoxSuccess != 1)
			printk("Error: NPU_WIFI_OFFLOAD Set RXD ring size fail\n");
	}	
}
EXPORT_SYMBOL_GPL(npu_init_rxD);
void wifi_offload_init_npu(struct pci_dev *pdev,struct mt7915_dev *dev,int flag)
{

	int isMailBoxSuccess = 0;
	unsigned char band_idx = 0,ring_idx = 0, pcidevindex = 0;	
	npu_info_t get_RingBase;
	dbg_counter_t get_CounterBase;
	unsigned short int node_index = 0, pop_index = 0, max_num;
	struct sk_buff *skb_pkt = NULL;
	unsigned long int pcie_pa, hcounter_pa;
	unsigned int PciePortType;
	struct hif_pci_rx_ring *rx_ring;
	unsigned long int reorder_node_pa;
	struct mt76_dev *mdev;
	mdev = &dev->mt76;
	struct mt7915_phy *phy;
	phy=&dev->phy;
	int countlen[3] ={47,44,15};
	int npuOffloadEn=1;
	u32 hif1_ofs = 0;
	int num,index;
	pcie_pa=NULL;
	if(npuOffloadEn){
		printk("npuOffload on mt76 init \n");	
		if (pdev->bus){
			band_idx = (pdev->bus->self->devfn >> 3) & 0x1f;	
			printk("the value of pdev->bus->self->devfn is %d ,and the band_index is %d\n",pdev->bus->self->devfn,band_idx);
			mdev->slot_id = (pdev->bus->self->devfn >> 3) & 0x1f;
		}
		if(dev_dbdc)
		{
			if(is_mt7916(mdev))
			{	
				band_idx = 0;
				isMailBoxSuccess = WIFI_MAIL_API_SET_WAIT_DRIVER_MODEL(band_idx, MT7916_D);
				if(isMailBoxSuccess != 1) 
					printk("Error: 7915D 111 NPU_WIFI_OFFLOAD Set Dirver Model Fail\n");
				isMailBoxSuccess = WIFI_MAIL_API_SET_WAIT_PCIE_PORT_TYPE(band_idx, P0__P0);
				if(isMailBoxSuccess != 1) 
					printk("Error: 7915A NPU_WIFI_OFFLOAD Set PCIE PORT Fail\n");	
			}
			else
			{	
				isMailBoxSuccess = WIFI_MAIL_API_SET_WAIT_DRIVER_MODEL(band_idx, MT7915_D);
				if(isMailBoxSuccess != 1) 
					printk("Error: 7915D 111 NPU_WIFI_OFFLOAD Set Dirver Model Fail\n");
				if(band_idx==0)//7915d use pcie0
					isMailBoxSuccess = WIFI_MAIL_API_SET_WAIT_PCIE_PORT_TYPE(band_idx, P0_);
				else//7915d use pcie1
					isMailBoxSuccess = WIFI_MAIL_API_SET_WAIT_PCIE_PORT_TYPE(band_idx, P1_);//xflu 0404
				if(isMailBoxSuccess != 1) 
					printk("Error: 7915A NPU_WIFI_OFFLOAD Set PCIE PORT Fail\n");					
			}
		}
				//DBG7915("Error: 7915D NPU_WIFI_OFFLOAD Set Dirver Model Fail\n");		
		else
		{
			isMailBoxSuccess = WIFI_MAIL_API_SET_WAIT_DRIVER_MODEL(band_idx, MT7915_A);
			if(isMailBoxSuccess != 1) 
				printk("Error: 7915A NPU_WIFI_OFFLOAD Set Dirver Model Fail\n");
			else
				printk("Error: 7915A NPU_WIFI_OFFLOAD Set Dirver Model success \n");

			isMailBoxSuccess = WIFI_MAIL_API_SET_WAIT_PCIE_PORT_TYPE(band_idx, P0__P0);
			if(isMailBoxSuccess != 1) 
				printk("Error: 7915A NPU_WIFI_OFFLOAD Set PCIE PORT Fail\n");
			else
				printk("Error: 7915A NPU_WIFI_OFFLOAD Set PCIE PORT success \n");

			
		}

		/****************set global setting(only once)**************/
		if((setonce != 1) && (setonce !=2))
			setonce = 1;

		if((setonce == 1)){
			printk("!!!! do phy and virtual address mapping\n");
			/**************debug count******************/

			struct device *pdev_for_cnt;
			
			pdev_for_cnt=&pdev->dev;
			HCounter_Base = dma_alloc_coherent(pdev_for_cnt, 0x200000, &hcounter_pa, GFP_KERNEL);
			memset(HCounter_Base, 0x0, (HOST_COUNTER_SIZE*sizeof(unsigned long int)));
			isMailBoxSuccess = WIFI_MAIL_API_GET_WAIT_DBG_COUNTER(3, &get_CounterBase); 
			NCounterU_Base = ioremap((phys_addr_t)get_CounterBase.errCount, (UTIL_COUNTER_NUM* sizeof(unsigned long int)));
			if(isMailBoxSuccess != 1) 
					printk("Error: 7915D NPU_WIFI_OFFLOAD Get counter base fail\n");

			for(index = 0; index < 2;index++)	
			{	
				isMailBoxSuccess = WIFI_MAIL_API_GET_WAIT_DBG_COUNTER(index, &get_CounterBase); 
				if(isMailBoxSuccess == 1) {

					if(index) 
						NCounter_Base = ioremap((phys_addr_t)get_CounterBase.errCount, (NPU_COUNTER_SIZE* sizeof(unsigned long int)));
					else 
						NCounter2G_Base = ioremap((phys_addr_t)get_CounterBase.errCount, (NPU_COUNTER_SIZE* sizeof(unsigned long int)));
				}
			}

			if(NCounter2G_Base)
				printk("the addr of NCounter2G_Base is %x \n",NCounter2G_Base);
			/**************ba node******************/
			dramReorderNodeBase =  dma_alloc_coherent(pdev_for_cnt, 0x200000, &reorder_node_pa, GFP_KERNEL);
			isMailBoxSuccess = WIFI_MAIL_API_SET_WAIT_DRAM_BA_NODE_ADDR(band_idx, reorder_node_pa);
			if(isMailBoxSuccess != 1)
				printk("Error: NPU_WIFI_OFFLOAD set BA Node in Dram base fail\n");
			setonce = 2;
		}
		pcie_pa = pci_resource_start(pdev, 0); //get pcie phy addr
		if(dev_dbdc)
		{
			if(is_mt7916(mdev))
			{
				printk("%s:%d mt7916d device !!!\n",__func__,__LINE__);
				if (dev->hif2)
					hif1_ofs = MT_WFDMA0_PCIE1(0) - MT_WFDMA0(0);
				for(num = 0; num < 2; num++){
					if(num)
						isMailBoxSuccess = WIFI_MAIL_API_SET_WAIT_PCIE_ADDR(num, (pcie_pa + MT_RXQ_RING_BASE(MT_RXQ_MAIN) + 0x50 + hif1_ofs));
					else
						isMailBoxSuccess = WIFI_MAIL_API_SET_WAIT_PCIE_ADDR(num, (pcie_pa + MT_RXQ_RING_BASE(MT_RXQ_MAIN) + 0x40));
				}
			}
			else
			{
				printk("%s:%d mt7915d device !!!\n",__func__,__LINE__);
				for(num = 0; num < 2; num++){
					if(num)
						isMailBoxSuccess = WIFI_MAIL_API_SET_WAIT_PCIE_ADDR(num, (pcie_pa + MT_RXQ_RING_BASE(MT_RXQ_MAIN) + 0x10));
					else
						isMailBoxSuccess = WIFI_MAIL_API_SET_WAIT_PCIE_ADDR(num, (pcie_pa + MT_RXQ_RING_BASE(MT_RXQ_MAIN)));
				}				
			}
		}
#if TCSUPPORT_NPU_WIFI_OFFLOAD || (defined(BB_SOC)&& defined(MULTI_INF_SUPPORT))
		else
		{
			printk("after :pci_resource_start= %x\n", pcie_pa);
			isMailBoxSuccess = WIFI_MAIL_API_SET_WAIT_PCIE_ADDR(band_idx, (pcie_pa+0xd4500));
			if(isMailBoxSuccess != 1) 
				printk("%s:%d 7915A NPU_WIFI_OFFLOAD Set pcie base fail\n",__func__,__LINE__);
		}
#endif
	}
}
EXPORT_SYMBOL_GPL(wifi_offload_init_npu);
void get_npu_rxd(struct mt7915_dev *dev,struct pci_dev *pdev)
{
	npu_info_t get_RingBase_2g, get_RingBase_5g,get_RingBase;
	int isMailBoxSuccess=0;
	unsigned char band_idx = 0;
	void __iomem *npu_regs;
	if (pdev->bus)
		band_idx = (pdev->bus->self->devfn >> 3) & 0x1f;
	if(dev_dbdc)
		band_idx = 0;
	isMailBoxSuccess = WIFI_MAIL_API_GET_WAIT_RXDESC_BASE(band_idx, &get_RingBase_2g);
	if(isMailBoxSuccess != 1) 
		printk("Error: NPU_WIFI_OFFLOAD get ring0 base fail\n");
	npu_regs=(NDIS_PHYSICAL_ADDRESS)get_RingBase_2g.info;
	writel(npu_regs,&dev->mt76.q_rx[MT_RXQ_MAIN].regs->desc_base);
	if(dev_dbdc)
	{	
		isMailBoxSuccess = WIFI_MAIL_API_GET_WAIT_RXDESC_BASE(1, &get_RingBase_5g);
		if(isMailBoxSuccess != 1) 
			printk("Error: NPU_WIFI_OFFLOAD get ring1 base fail\n");
		npu_regs=(NDIS_PHYSICAL_ADDRESS)get_RingBase_5g.info;
		writel(npu_regs,&dev->mt76.q_rx[MT_RXQ_BAND1].regs->desc_base);
	}
}
EXPORT_SYMBOL_GPL(get_npu_rxd);


int npu_enable(void)
{
	return npu_enable_chk;
}
void set_npu_enable(int enable)
{
	npu_enable_chk = enable;
}
EXPORT_SYMBOL_GPL(npu_enable);
EXPORT_SYMBOL_GPL(set_npu_enable);


int mt7915_npu_count(struct seq_file *s, void *data)
{
	//struct mt7915_dev *dev = dev_get_drvdata(s->private);
	//struct mt76_dev *mdev = NULL;
	seq_printf(s, "Version: 2.2.6.0\n");


	printk("show host pkt count:\n");
	printk("------Host Counter----------\n");
	printk("HOST_WHILE_COUNTER:\t\t%lu\n", HOST_COUNTER(HOST_WHILE_COUNT));
	printk("RESCHEDULE_COUNT:\t\t%lu\n", HOST_COUNTER(RESCHEDULE_COUNT));
	printk("PKT_NULL_COUNT:\t\t\t%lu\n",HOST_COUNTER(PKT_NULL_COUNT));
	printk("HOST_GET_2GPKT_COUNT:\t\t%lu\n", HOST_COUNTER(HOST_GET_2GPKT_COUNT));
	printk("HOST_GET_5GPKT_COUNT:\t\t%lu\n",HOST_COUNTER(HOST_GET_5GPKT_COUNT));

	//printk("HOST_WHILE_COUNTER:\t\t%lu\nRESCHEDULE_COUNT:\t\t%lu\nNOT_DATA_2GPKT_COUNT:\t\t%lu\nNOT_DATA_5GPKT_COUNT:\t\t%lu\nHAS_CRESN_2GPKT_COUNT:\t\t%lu\nDEQ_PKT_COUNT:\t\t\t%lu\nPKT_NULL_COUNT:\t\t\t%lu\nHOST_GET_2GPKT_COUNT:\t\t%lu\nHOST_GET_5GPKT_COUNT:\t\t%lu\n", HOST_COUNTER(HOST_WHILE_COUNT), HOST_COUNTER(RESCHEDULE_COUNT), HOST_COUNTER(NOT_DATA_PKT_COUNT), HOST_COUNTER(HAS_CRESN_PKT_COUNT), HOST_COUNTER(DEQ_2GPKT_COUNT), HOST_COUNTER(DEQ_5GPKT_COUNT),HOST_COUNTER(PKT_NULL_COUNT), HOST_COUNTER(HOST_GET_2GPKT_COUNT), HOST_COUNTER(HOST_GET_5GPKT_COUNT));
	printk("========== NPU 5G Counter ===========\n");
	printk("get packet while count:\t\t%lu\n", NPU_COUNTER(NCounter_Base,WHILE_COUNT));
	printk("RX_DESC_DDONE\t\t\t%lu\n", NPU_COUNTER(NCounter_Base,RX_DESC_DDONE));
	printk("ALL_GET_PKT_COUNT\t\t%lu\n", NPU_COUNTER(NCounter_Base,ALL_GET_PKT_COUNT));
	//printk("ACCESS_PACKET\t\t\t%lu\n", NPU_COUNTER(NCounter_Base,ACCESS_PACKET));
	printk("DROP_PACKETS\t\t\t%lu\n", NPU_COUNTER(NCounter_Base,DROP_PACKETS));
	printk("TO_QDMA_COUNT\t\t\t%lu\n", NPU_COUNTER(NCounter_Base,TO_QDMA_COUNT));
	//printk("ALL_ENQ_PKT_COUNT\t\t%lu\n", NPU_COUNTER(NCounter_Base,ALL_ENQ_PKT_COUNT));
	//printk("ENQ_BUFID_FIAL_COUNT\t\t%lu\n", NPU_COUNTER(NCounter_Base,ENQ_BUFID_FIAL_COUNT));
	//printk("ENQ_BUFID_ERROR_COUNT:\t\t%lu\n",NPU_COUNTER(NCounter_Base, ENQ_BUFID_ERROR_COUNT));
	printk("GET_BUFID_FAIL\t\t\t%lu\n", NPU_COUNTER(NCounter_Base,GET_BUFID_FAIL));
	//printk("NO_BUFID\t\t\t%lu\n", NPU_COUNTER(NCounter_Base,NO_BUFID));
	printk("SCATTER_CNT_MORE1\t\t%lu\n", NPU_COUNTER(NCounter_Base, SCATTER_CNT_MORE1));
	printk("BIGGER_PACKET\t\t\t%lu\n", NPU_COUNTER(NCounter_Base,BIGGER_PACKET));
	
	printk("------Enq & Deq Counter----------\n");
	printk("ENQ_DEQ_WHILE_COUNT\t\t%lu\n", NPU_COUNTER(NCounter_Base,ENQ_DEQ_WHILE_COUNT));
	printk("ENQ_SRAM_COUNT:\t\t\t%lu\n",NPU_COUNTER(NCounter_Base,ENQ_SRAM_COUNT));
	printk("ENQ_SRAM_FULL_COUNT:\t\t%lu\n",NPU_COUNTER(NCounter_Base,ENQ_SRAM_FULL_COUNT));
	printk("ENQ_SRAM_FAIL_COUNT:\t\t%lu\n",NPU_COUNTER(NCounter_Base,ENQ_SRAM_FAIL_COUNT));
	printk("DEQ_SRAM_COUNT:\t\t\t%lu\nDEQ_SRAM_FAIL_BUFID_COUNT:\t%lu\nDEQ_SRAM_FAIL_LEN_COUNT:\t%lu\n",NPU_COUNTER(NCounter_Base,DEQ_SRAM_COUNT),NPU_COUNTER(NCounter_Base, DEQ_SRAM_FAIL_BUFID_COUNT), NPU_COUNTER(NCounter_Base,DEQ_SRAM_FAIL_LEN_COUNT));
	//printk("DEQ_SRAM_NO_INFO_COUNT\t\t%lu\n", NPU_COUNTER(NCounter_Base, DEQ_SRAM_NO_INFO_COUNT));
	printk("TO_HOSTAPD_COUNT\t\t\t%lu\n", NPU_COUNTER(NCounter_Base, TO_HOSTAPD_COUNT));
	//printk("ENQ_DRAM_FAIL_COUNT:\t\t%lu\n",NPU_COUNTER(NCounter_Base, ENQ_DRAM_FAIL_COUNT));
	printk("HOST_APD_ERROR_COUNT:\t\t%lu\n", NPU_COUNTER(NCounter_Base, HOST_APD_ERROR_COUNT));

	printk("------BA Counter----------\n");
	printk("BA1(in order):\t\t\t%lu\nBA2(Dupl Packet):\t\t%lu\nBA3(old packet):\t\t%lu\nBA4(with in window):\t\t%lu\nBA5(surpasses Win):\t\t%lu\nBA_AMSDU:\t\t\t%lu\n!AMPDU_COUNT:\t\t\t%lu\n",NPU_COUNTER(NCounter_Base, BA_IN_ORDER_PKT_COUNT),NPU_COUNTER(NCounter_Base, BA_DUPL_PKT_COUNT), NPU_COUNTER(NCounter_Base,BA_OLD_PKT_COUNT), NPU_COUNTER(NCounter_Base,BA_WITHIN_WS_PKT_COUNT), NPU_COUNTER(NCounter_Base, BA_POP_PKT_COUNT), NPU_COUNTER(NCounter_Base, BA_AMSDU_COUNT),  NPU_COUNTER(NCounter_Base, AMPDU_COUNT));
	//printk("BA_NO_PKT_IN_LIST_COUNT:\t%lu\n", NPU_COUNTER(NCounter_Base, BA_NO_PKT_IN_LIST_COUNT));
	//printk("BA_NOT_DATA_PKT_COUNT:\t\t%lu\n", NPU_COUNTER(NCounter_Base, BA_NOT_DATA_PKT_COUNT));
	printk("BA_NO_MEM_COUNT:\t\t%lu\n", NPU_COUNTER(NCounter_Base, BA_NO_MEM_COUNT));
	//printk("BA_NODE_ALLOC_COUNT:\t\t%lu\n", NPU_COUNTER(NCounter_Base, BA_NODE_ALLOC_COUNT));
	//printk("BA_NODE_ALLOC_FAIL:\t\t%lu\n", NPU_COUNTER(NCounter_Base, BA_NODE_ALLOC_FAIL));
	printk("BA_AMSDU_MISS:\t\t\t%lu\n", NPU_COUNTER(NCounter_Base, BA_AMSDU_MISS));
	//printk("BA_TIMEOUT_FLUSH:\t\t%lu\n", NPU_COUNTER(NCounter_Base, BA_TIMEOUT_FLUSH));
	printk("BA_TIMEOUT_FLUSH100:\t\t%lu\n", NPU_COUNTER(NCounter_Base, BA_TIMEOUT_FLUSH100));
	printk("BA_TIMEOUT_FLUSH250:\t\t%lu\n", NPU_COUNTER(NCounter_Base, BA_TIMEOUT_FLUSH250));
	printk("BA_ENQ_DUP_SEQ:\t\t\t%lu\n", NPU_COUNTER(NCounter_Base, BA_ENQ_DUP_SEQ));
	printk("BA_REORDERING_NODE_ALLOC_COUNT\t%lu\n", NPU_COUNTER(NCounter_Base,BA_REORDERING_NODE_ALLOC_COUNT));
	printk("BA_REORDERING_NODE_ALLOC_DRAM_COUNT\t%lu\n", NPU_COUNTER(NCounter_Base,BA_REORDERING_NODE_ALLOC_DRAM_COUNT));
	printk("BA_REORDERING_NODE_FREE_COUNT\t%lu\n", NPU_COUNTER(NCounter_Base,BA_REORDERING_NODE_FREE_COUNT));
	printk("BA_REORDERINT_NODE_ALLOC_FAIL\t%lu\n", NPU_COUNTER(NCounter_Base,BA_REORDERINT_NODE_ALLOC_FAIL));

	//printk("------QDMA return Counter----------\n");
	//printk("QDMA_RETURN_UNBIND_COUNT:\t%lu\n", NPU_COUNTER(NCounter_Base, QDMA_RETURN_UNBIND_COUNT));
	//printk("QDMA_RETURN_FAIL_COUNT:\t\t%lu\n", NPU_COUNTER(NCounter_Base, QDMA_RETURN_FAIL_COUNT));
	//printk("QDMA_RETURN_OK_COUNT:\t\t%lu\n", NPU_COUNTER(NCounter_Base, QDMA_RETURN_OK_COUNT));

	printk("========== NPU 2.4G Counter ===========\n");
	printk("get packet while count:\t\t%lu\n", NPU_COUNTER(NCounter2G_Base,WHILE_COUNT));
	printk("RX_DESC_DDONE\t\t\t%lu\n", NPU_COUNTER(NCounter2G_Base,RX_DESC_DDONE));
	printk("ALL_GET_PKT_COUNT\t\t%lu\n", NPU_COUNTER(NCounter2G_Base,ALL_GET_PKT_COUNT));
	//printk("ACCESS_PACKET\t\t\t%lu\n", NPU_COUNTER(NCounter2G_Base,ACCESS_PACKET));
	printk("DROP_PACKETS\t\t\t%lu\n", NPU_COUNTER(NCounter2G_Base,DROP_PACKETS));
	printk("TO_QDMA_COUNT\t\t\t%lu\n", NPU_COUNTER(NCounter2G_Base,TO_QDMA_COUNT));
	//printk("ALL_ENQ_PKT_COUNT\t\t%lu\n", NPU_COUNTER(NCounter2G_Base,ALL_ENQ_PKT_COUNT));
	//printk("ENQ_BUFID_FIAL_COUNT\t\t%lu\n", NPU_COUNTER(NCounter2G_Base,ENQ_BUFID_FIAL_COUNT));
	//printk("ENQ_BUFID_ERROR_COUNT:\t\t%lu\n",NPU_COUNTER(NCounter2G_Base,ENQ_BUFID_ERROR_COUNT));
	printk("GET_BUFID_FAIL\t\t\t%lu\n", NPU_COUNTER(NCounter2G_Base,GET_BUFID_FAIL));
	//printk("NO_BUFID\t\t\t%lu\n", NPU_COUNTER(NCounter2G_Base,NO_BUFID));
	printk("SCATTER_CNT_MORE1\t\t%lu\n", NPU_COUNTER(NCounter2G_Base, SCATTER_CNT_MORE1));
	printk("BIGGER_PACKET\t\t\t%lu\n", NPU_COUNTER(NCounter2G_Base,BIGGER_PACKET));
	
	printk("------Enq & Deq Counter----------\n");
	printk("ENQ_DEQ_WHILE_COUNT\t\t%lu\n", NPU_COUNTER(NCounter2G_Base,ENQ_DEQ_WHILE_COUNT));
	printk("ENQ_SRAM_COUNT:\t\t\t%lu\n",NPU_COUNTER(NCounter2G_Base,ENQ_SRAM_COUNT));
	printk("ENQ_SRAM_FULL_COUNT:\t\t%lu\n",NPU_COUNTER(NCounter2G_Base,ENQ_SRAM_FULL_COUNT));
	printk("ENQ_SRAM_FAIL_COUNT:\t\t%lu\n",NPU_COUNTER(NCounter2G_Base,ENQ_SRAM_FAIL_COUNT));
	printk("DEQ_SRAM_COUNT:\t\t\t%lu\nDEQ_SRAM_FAIL_BUFID_COUNT:\t%lu\nDEQ_SRAM_FAIL_LEN_COUNT:\t%lu\n",NPU_COUNTER(NCounter2G_Base,DEQ_SRAM_COUNT),NPU_COUNTER(NCounter2G_Base, DEQ_SRAM_FAIL_BUFID_COUNT), NPU_COUNTER(NCounter2G_Base,DEQ_SRAM_FAIL_LEN_COUNT));
	//printk("DEQ_SRAM_NO_INFO_COUNT\t\t%lu\n", NPU_COUNTER(NCounter2G_Base, DEQ_SRAM_NO_INFO_COUNT));
	printk("TO_HOSTAPD_COUNT:\t\t\t%lu\n", NPU_COUNTER(NCounter2G_Base, TO_HOSTAPD_COUNT));
	//printk("ENQ_DRAM_FAIL_COUNT:\t\t%lu\n",NPU_COUNTER(NCounter2G_Base, ENQ_DRAM_FAIL_COUNT));
	printk("HOST_APD_ERROR_COUNT:\t\t%lu\n", NPU_COUNTER(NCounter2G_Base, HOST_APD_ERROR_COUNT));

	printk("------BA Counter----------\n");
	printk("BA1(in order):\t\t\t%lu\nBA2(Dupl Packet):\t\t%lu\nBA3(old packet):\t\t%lu\nBA4(with in window):\t\t%lu\nBA5(surpasses Win):\t\t%lu\nBA_AMSDU:\t\t\t%lu\n!AMPDU_COUNT:\t\t\t%lu\n",NPU_COUNTER(NCounter2G_Base, BA_IN_ORDER_PKT_COUNT),NPU_COUNTER(NCounter2G_Base, BA_DUPL_PKT_COUNT), NPU_COUNTER(NCounter2G_Base,BA_OLD_PKT_COUNT), NPU_COUNTER(NCounter2G_Base,BA_WITHIN_WS_PKT_COUNT), NPU_COUNTER(NCounter2G_Base, BA_POP_PKT_COUNT), NPU_COUNTER(NCounter2G_Base, BA_AMSDU_COUNT),  NPU_COUNTER(NCounter2G_Base, AMPDU_COUNT));
	//printk("BA_NO_PKT_IN_LIST_COUNT:\t%lu\n", NPU_COUNTER(NCounter2G_Base, BA_NO_PKT_IN_LIST_COUNT));
	//printk("BA_NOT_DATA_PKT_COUNT:\t\t%lu\n", NPU_COUNTER(NCounter2G_Base, BA_NOT_DATA_PKT_COUNT));
	printk("BA_NO_MEM_COUNT:\t\t%lu\n", NPU_COUNTER(NCounter2G_Base, BA_NO_MEM_COUNT));
	//printk("BA_NODE_ALLOC_COUNT:\t\t%lu\n", NPU_COUNTER(NCounter2G_Base, BA_NODE_ALLOC_COUNT));
	//printk("BA_NODE_ALLOC_FAIL:\t\t%lu\n", NPU_COUNTER(NCounter2G_Base, BA_NODE_ALLOC_FAIL));
	printk("BA_AMSDU_MISS:\t\t\t%lu\n", NPU_COUNTER(NCounter2G_Base, BA_AMSDU_MISS));
	//printk("BA_TIMEOUT_FLUSH:\t\t%lu\n", NPU_COUNTER(NCounter2G_Base, BA_TIMEOUT_FLUSH));
	printk("BA_TIMEOUT_FLUSH100:\t\t%lu\n", NPU_COUNTER(NCounter2G_Base, BA_TIMEOUT_FLUSH100));
	printk("BA_TIMEOUT_FLUSH250:\t\t%lu\n", NPU_COUNTER(NCounter2G_Base, BA_TIMEOUT_FLUSH250));
	printk("BA_ENQ_DUP_SEQ:\t\t\t%lu\n", NPU_COUNTER(NCounter2G_Base, BA_ENQ_DUP_SEQ));
	printk("BA_REORDERING_NODE_ALLOC_COUNT\t%lu\n", NPU_COUNTER(NCounter2G_Base,BA_REORDERING_NODE_ALLOC_COUNT));
	printk("BA_REORDERING_NODE_ALLOC_DRAM_COUNT\t%lu\n", NPU_COUNTER(NCounter2G_Base,BA_REORDERING_NODE_ALLOC_DRAM_COUNT));
	printk("BA_REORDERING_NODE_FREE_COUNT\t%lu\n", NPU_COUNTER(NCounter2G_Base,BA_REORDERING_NODE_FREE_COUNT));
	printk("BA_REORDERINT_NODE_ALLOC_FAIL\t%lu\n", NPU_COUNTER(NCounter2G_Base,BA_REORDERINT_NODE_ALLOC_FAIL));

	//printk("------QDMA return Counter----------\n");
	//printk("QDMA_RETURN_UNBIND_COUNT:\t%lu\n", NPU_COUNTER(NCounter2G_Base, QDMA_RETURN_UNBIND_COUNT));
	//printk("QDMA_RETURN_FAIL_COUNT:\t\t%lu\n", NPU_COUNTER(NCounter2G_Base, QDMA_RETURN_FAIL_COUNT));
	//printk("QDMA_RETURN_OK_COUNT:\t\t%lu\n", NPU_COUNTER(NCounter_Base, QDMA_RETURN_OK_COUNT));

	printk("=======================================\n");
	printk("============ QDMA Counter =============\n");
	
	printk("QDMA_TXDESC_NULL_COUNT:\t\t%lu\n", NPU_COUNTER(NCounterU_Base,QDMA_TXDESC_NULL_COUNT));
	printk("QDMA_UNBIND_COUNT:\t\t%lu\n", NPU_COUNTER(NCounterU_Base,QDMA_UNBIND_COUNT));
	//printk("QDMA_TO_PPE_COUNT:\t\t%lu\n", NPU_COUNTER(NCounterU_Base,QDMA_TO_PPE_COUNT));
	printk("QDMA_DONE_COUNT:\t\t%lu\n", NPU_COUNTER(NCounterU_Base,QDMA_DONE_COUNT));
	printk("QDMA_FREE_BUFID_COUNT:\t\t%lu\n", NPU_COUNTER(NCounterU_Base,QDMA_FREE_BUFID_COUNT));
	printk("QDMA_TO_ENQ_FAIL_COUNT:\t\t%lu\n", NPU_COUNTER(NCounterU_Base,QDMA_TO_ENQ_FAIL_COUNT));
	//printk("QDMA_TO_ENQ_FAIL_COUNT2:\t%lu\n", NPU_COUNTER(NCounterU_Base,QDMA_TO_ENQ_FAIL_COUNT2));
	printk("QDMA_TX_DSCP_IDX_INVALID:\t\t%lu\n", NPU_COUNTER(NCounterU_Base,QDMA_TX_DSCP_IDX_INVALID));
	printk("QDMA_TX_DSCP_INFO_ERROR:\t\t%lu\n", NPU_COUNTER(NCounterU_Base,QDMA_TX_DSCP_INFO_ERROR));
	printk("QDMA_DONE_DROP_BIT_ERROR:\t\t%lu\n", NPU_COUNTER(NCounterU_Base,QDMA_DONE_DROP_BIT_ERROR));
	//printk("QDMA_TXDESC_PUSH_COUNT:\t\t%lu\n", NPU_COUNTER(NCounterU_Base,QDMA_TXDESC_PUSH_COUNT));
	//printk("QDMA_TXDESC_PUSH2_COUNT:\t\t%lu\n", NPU_COUNTER(NCounterU_Base,QDMA_TXDESC_PUSH2_COUNT));
	//printk("QDMA_TXDESC_POP_COUNT:\t\t%lu\n", NPU_COUNTER(NCounterU_Base,QDMA_TXDESC_POP_COUNT));

	printk("============ Bufid Counter ==========\n");
	printk("SKB_ALLOC_BUFID_COUNT\t\t%lu\n", NPU_COUNTER(NCounterU_Base,SKB_ALLOC_BUFID_COUNT));
	printk("SKB_FREE_BUFID_COUNT\t\t%lu\n", NPU_COUNTER(NCounterU_Base,SKB_FREE_BUFID_COUNT));
	printk("SKB_BUFID_ALLOC_FAIL\t\t%lu\n", NPU_COUNTER(NCounterU_Base,SKB_BUFID_ALLOC_FAIL));

	return 0;
}
EXPORT_SYMBOL_GPL(mt7915_npu_count);



#endif
