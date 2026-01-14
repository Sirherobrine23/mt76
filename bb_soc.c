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

	if(is_dbdc){
		snprintf(file_path, 100, "%s", BIN_FILE_PATH);
	}
	else{
		if(glb_eeprom_has_read_once){
			snprintf(file_path, 100, "%s", BIN_FILE_PATH);
		}
		else{
			glb_eeprom_has_read_once = true;
			snprintf(file_path, 100, "%s", BIN_FILE_PATH_2G);
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
