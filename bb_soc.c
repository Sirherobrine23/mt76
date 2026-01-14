/*
	Module Name:
	bb_soc.c

	Abstract:
	Broad Band SoC platform added.

	revision History:
	Who        When          What
	---------  ----------    ----------------------------------------------
	GeoYang		2021.Apr.13		file created
 */
#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/proc_fs.h>

#include "mt76.h"

//set the file path to be the same as Linux driver
//maybe one day mt76+mac80211.ko is porting to tclinux
#define BIN_FILE_PATH			"/etc/Wireless/RT2860AP_AC/RT30xxEEPROM.bin"
#define BIN_FILE_PATH_2G		"/etc/RT30xxEEPROM.bin"
#define FILE_OPEN_ERR(_fd)		((_fd == NULL) || IS_ERR((_fd)))

static bool glb_eeprom_has_read_once = false;



static int bbsoc_file_read(struct file *fp, char *eeprom_data, int len)
{
// > 3.19.0
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3, 19, 0))
	if (fp->f_op && fp->f_op->read) {
		return fp->f_op->read(fp, eeprom_data, len, &fp->f_pos);
	}
// 3.19.1 ~ 4.19.0
#elif (LINUX_VERSION_CODE < KERNEL_VERSION(4, 19, 0))
	if (fp->f_mode & FMODE_CAN_READ) {
		return __vfs_read(fp, eeprom_data, len, &fp->f_pos);
	}
// > 4.19.1
#else
	if (fp->f_mode & FMODE_CAN_READ) {
		return kernel_read(fp, eeprom_data, len, &fp->f_pos);
	}
#endif
	else{
		printk("mt76: this should be an error !!!\n");
	}

	return -EPERM;
}

/*
 * Implementation for reading eeprom from bin file instead of using mtd_read
 */

#define MT7916D_IPAILNA_path "/etc/Wireless/RT2860AP_AC/MT7916D_iPAiLNA.bin"
#define MT7915D_IPAILNA_path "/etc/Wireless/RT2860AP_AC/MT7915D_iPAiLNA.bin"
#define MT7915A_BAND0_IPAILNA_path "/etc/MT7915A_band0_iPAiLNA.bin"
#define MT7915A_BAND1_IPAILNA_path "/etc/Wireless/RT2860AP_AC/MT7915A_band1_iPAiLNA.bin"


#define MT7915D_IPAELNA_path "/etc/Wireless/RT2860AP_AC/MT7915D_IPAELNA.bin"
#define MT7915A_BAND0_IPAELNA_path "/etc/MT7915A_band0_IPAELNA.bin"
#define MT7915A_BAND1_IPAELNA_path "/etc/Wireless/RT2860AP_AC/MT7915A_band1_IPAELNA.bin"

#define MT7916D_EPAELNA_path "/etc/Wireless/RT2860AP_AC/MT7916D_EPAELNA.bin"
#define MT7915D_EPAELNA_path "/etc/Wireless/RT2860AP_AC/MT7915D_EPAELNA.bin"
#define MT7915A_BAND0_EPAELNA_path "/etc/MT7915A_band0_EPAELNA.bin"
#define MT7915A_BAND1_EPAELNA_path "/etc/Wireless/RT2860AP_AC/MT7915A_band1_EPAELNA.bin"

#define MT7915A_BAND0_EPAILNA_path "/etc/MT7915A_band0_EPAILNA.bin"
#define MT7915A_BAND1_EPAILNA_path "/etc/Wireless/RT2860AP_AC/MT7915A_band1_iPAiLNA.bin"

static char wifi_chip_type[32]={0};
static inline bool is_mt7915(struct mt76_dev *dev)
{
	return mt76_chip(dev) == 0x7915;
}
static inline bool is_mt7916(struct mt76_dev *dev)
{
	return mt76_chip(dev) == 0x7906;
}
static int chip_type_check(void)
{
	char file_path[100] = {'\0'};
	int ret;
	struct file *filp_r = NULL;
	mm_segment_t orgfs;
	snprintf(file_path, 100, "%s", "/etc/wifi_chip_type");
	orgfs = get_fs();
	set_fs(KERNEL_DS);
	filp_r = filp_open(file_path, O_RDONLY, 0);	
	if(FILE_OPEN_ERR(filp_r)){
		printk("%s:%d open error !!!\n",__func__,__LINE__);
		set_fs(orgfs);
		return 0;
	}	
	ret = bbsoc_file_read(filp_r, wifi_chip_type, 32);
	if(ret > 0) {
		if(strstr(wifi_chip_type,"IPAILNA"))
			printk("%s:%d wifi_chip_type=%s !\n",__func__,__LINE__,wifi_chip_type);
	}
	filp_close(filp_r, NULL);
	set_fs(orgfs);
	return 0;	
}

static int band_calibrated(int band)
{
	char file_path[100] = {'\0'};
	char tempStr[64] = {0};
	int ret, result=0;
	struct file *filp_r = NULL;
	mm_segment_t orgfs;
	if(band)
		snprintf(file_path, 100, "%s", "/etc/band1_calibrated");
	else
		snprintf(file_path, 100, "%s", "/etc/band0_calibrated");
	orgfs = get_fs();
	set_fs(KERNEL_DS);
	filp_r = filp_open(file_path, O_RDONLY, 0);	
	if(FILE_OPEN_ERR(filp_r)){
		printk("%s:%d open error !!!\n",__func__,__LINE__);
		return 0;
	}	
	ret = bbsoc_file_read(filp_r, tempStr, 64);
	if(ret > 0) {
		if(strstr(tempStr,"1"))
		{
			printk("%s:%d %s has calibrated!\n",__func__,__LINE__,band?"5G":"2G");
			result = 1;
		}
	}
	filp_close(filp_r, NULL);
	set_fs(orgfs);
	return result;
}


int bbsoc_load_eeprom_from_bin(struct mt76_dev *dev, int len, bool is_dbdc)
{
	char file_path[100] = {'\0'};
	mm_segment_t	oldfs;
	struct file *fp = NULL;
	int ret = 0;


	dev->eeprom.size = len;
	dev->eeprom.data = devm_kzalloc(dev->dev, len, GFP_KERNEL);
	if (!dev->eeprom.data){
		dev_info(dev->dev,"mt76 mem alloc fail!!!\n");
		return -ENOMEM;
	}

	memset(dev->eeprom.data, 0, len);
	//dev_info(dev->dev,"%s() len:%d is_dbdc:%d\n", __func__, len, is_dbdc);
	chip_type_check();
	if(is_dbdc){
		if(band_calibrated(1))
			snprintf(file_path, 100, "%s", BIN_FILE_PATH);
		else{
			if(!is_mt7915(dev) && !(is_mt7916(dev)))
			{
				printk("%s:%d chip error !!!\n",__func__,__LINE__);
				return -ENOMEM;	
			}
			if(strstr(wifi_chip_type,"IPAILNA"))
			{
				if(is_mt7916(dev))
					snprintf(file_path, 100, "%s", MT7916D_IPAILNA_path);
				else
					snprintf(file_path, 100, "%s", MT7915D_IPAILNA_path);
			}
			else if(strstr(wifi_chip_type,"IPAELNA"))
			{
				if(is_mt7916(dev))
				{
					printk("%s:%d chip error !!!\n",__func__,__LINE__);
					return -ENOMEM;					
				}
				else
					snprintf(file_path, 100, "%s", MT7915D_IPAELNA_path);
			}
			else if(strstr(wifi_chip_type,"EPAELNA"))
			{
				if(is_mt7916(dev))
					snprintf(file_path, 100, "%s", MT7916D_EPAELNA_path);
				else
					snprintf(file_path, 100, "%s", MT7915D_EPAELNA_path);			
			}
			else{
				printk("%s:%d use default!!!\n",__func__,__LINE__);
				snprintf(file_path, 100, "%s", BIN_FILE_PATH);			
			}
		}
	}
	else{
		if(!is_mt7915(dev))
		{
			printk("%s:%d chip error !!!\n",__func__,__LINE__);
			return -ENOMEM;					
		}
		if(glb_eeprom_has_read_once){
			if(band_calibrated(1))
				snprintf(file_path, 100, "%s", BIN_FILE_PATH);
			else{
				if(strstr(wifi_chip_type,"IPAILNA"))
					snprintf(file_path, 100, "%s", MT7915A_BAND1_IPAILNA_path);
				else if(strstr(wifi_chip_type,"IPAELNA"))
					snprintf(file_path, 100, "%s", MT7915A_BAND1_IPAELNA_path);
				else if(strstr(wifi_chip_type,"EPAELNA"))
					snprintf(file_path, 100, "%s", MT7915A_BAND1_EPAELNA_path);
				else if(strstr(wifi_chip_type,"EPAILNA"))
					snprintf(file_path, 100, "%s", MT7915A_BAND1_EPAILNA_path);
				else
				{
					printk("%s:%d use default !!!\n",__func__,__LINE__);
					snprintf(file_path, 100, "%s", BIN_FILE_PATH);
				}
			}
		}
		else{
			glb_eeprom_has_read_once = true;
			if(band_calibrated(0))
				snprintf(file_path, 100, "%s", BIN_FILE_PATH_2G);
			else{
				if(strstr(wifi_chip_type,"IPAILNA"))
					snprintf(file_path, 100, "%s", MT7915A_BAND0_IPAILNA_path);
				else if(strstr(wifi_chip_type,"IPAELNA"))
					snprintf(file_path, 100, "%s", MT7915A_BAND0_IPAELNA_path);
				else if(strstr(wifi_chip_type,"EPAELNA"))
					snprintf(file_path, 100, "%s", MT7915A_BAND0_EPAELNA_path);
				else if(strstr(wifi_chip_type,"EPAILNA"))
					snprintf(file_path, 100, "%s", MT7915A_BAND0_EPAILNA_path);
				else
				{
					printk("%s:%d use default !!!\n",__func__,__LINE__);
					snprintf(file_path, 100, "%s", BIN_FILE_PATH_2G);
				}
			}
		}
	}
	dev_info(dev->dev,"%s() file_path:%s\n", __func__, file_path);

	// init
	oldfs = get_fs();
	set_fs(KERNEL_DS);

	// file open
	fp = filp_open(file_path, O_RDONLY, 0);
	if(FILE_OPEN_ERR(fp)){
		dev_info(dev->dev,"open file fail: %s\n", file_path);
		goto error_condition;
	}

	// file read
	ret = bbsoc_file_read(fp, dev->eeprom.data, len);
	if(ret > 0) {
		dev_info(dev->dev,"load eeprom succesfully from %s\n", file_path);
	}
	else {
		dev_info(dev->dev,"read file fail: %s\n", file_path);
		filp_close(fp, NULL);
		goto error_condition;
	}


	// file close
	filp_close(fp, NULL);
	set_fs(oldfs);

	return 0;

error_condition:
	set_fs(oldfs);
	return -ENOENT;
}
EXPORT_SYMBOL_GPL(bbsoc_load_eeprom_from_bin);
