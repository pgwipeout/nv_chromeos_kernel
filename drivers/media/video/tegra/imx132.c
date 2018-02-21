/*
 * imx132.c - imx132 sensor driver
 *
 * Copyright (c) 2012, NVIDIA, All Rights Reserved.
 *
 * Contributors:
 *      Krupal Divvela <kdivvela@nvidia.com>
 *
 *
 * This file is licensed under the terms of the GNU General Public License
 * version 2. This program is licensed "as is" without any warranty of any
 * kind, whether express or implied.
 */

#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/i2c.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/regulator/consumer.h>
#include <media/imx132.h>
#include <linux/gpio.h>
#include <linux/module.h>
#include <linux/moduleparam.h>

struct imx132_reg {
	u16 addr;
	u16 val;
};

struct imx132_info {
	struct miscdevice		miscdev_info;
	int				mode;
	struct imx132_power_rail	power;
	struct imx132_sensordata	sensor_data;
	struct i2c_client		*i2c_client;
	struct imx132_platform_data	*pdata;
	atomic_t			in_use;
};

#define IMX132_TABLE_WAIT_MS 0
#define IMX132_TABLE_END 1

#define IMX132_WAIT_MS 5


static struct imx132_reg mode_1976x1200[] = {
	/* Stand by */
	{0x0100, 0x00},
	{IMX132_TABLE_WAIT_MS, IMX132_WAIT_MS},

	/* global settings */
	{0x3087, 0x53},
	{0x308B, 0x5A},
	{0x3094, 0x11},
	{0x309D, 0xA4},
	{0x30AA, 0x01},
	{0x30C6, 0x00},
	{0x30C7, 0x00},
	{0x3118, 0x2F},
	{0x312A, 0x00},
	{0x312B, 0x0B},
	{0x312C, 0x0B},
	{0x312D, 0x13},

	/* PLL Setting */
	{0x0305, 0x02},
	{0x0307, 0x21},
	{0x30A4, 0x02},
	{0x303C, 0x4B},

	/* Mode Setting */
	{0x0340, 0x04},
	{0x0341, 0xCA},
	{0x0342, 0x08},
	{0x0343, 0xC8},
	{0x0344, 0x00},
	{0x0345, 0x00},
	{0x0346, 0x00},
	{0x0347, 0x00},
	{0x0348, 0x07},
	{0x0349, 0xB7},
	{0x034A, 0x04},
	{0x034B, 0xAF},
	{0x034C, 0x07},
	{0x034D, 0xB8},
	{0x034E, 0x04},
	{0x034F, 0xB0},
	{0x0381, 0x01},
	{0x0383, 0x01},
	{0x0385, 0x01},
	{0x0387, 0x01},
	{0x303D, 0x10},
	{0x303E, 0x4A},
	{0x3040, 0x08},
	{0x3041, 0x97},
	{0x3048, 0x00},
	{0x304C, 0x2F},
	{0x304D, 0x02},
	{0x3064, 0x92},
	{0x306A, 0x10},
	{0x309B, 0x00},
	{0x309E, 0x41},
	{0x30A0, 0x10},
	{0x30A1, 0x0B},
	{0x30B2, 0x00},
	{0x30D5, 0x00},
	{0x30D6, 0x00},
	{0x30D7, 0x00},
	{0x30D8, 0x00},
	{0x30D9, 0x00},
	{0x30DA, 0x00},
	{0x30DB, 0x00},
	{0x30DC, 0x00},
	{0x30DD, 0x00},
	{0x30DE, 0x00},
	{0x3102, 0x0C},
	{0x3103, 0x33},
	{0x3104, 0x30},
	{0x3105, 0x00},
	{0x3106, 0xCA},
	{0x3107, 0x00},
	{0x3108, 0x06},
	{0x3109, 0x04},
	{0x310A, 0x04},
	{0x315C, 0x3D},
	{0x315D, 0x3C},
	{0x316E, 0x3E},
	{0x316F, 0x3D},
	{0x3301, 0x00},
	{0x3304, 0x07},
	{0x3305, 0x06},
	{0x3306, 0x19},
	{0x3307, 0x03},
	{0x3308, 0x0F},
	{0x3309, 0x07},
	{0x330A, 0x0C},
	{0x330B, 0x06},
	{0x330C, 0x0B},
	{0x330D, 0x07},
	{0x330E, 0x03},
	{0x3318, 0x67},
	{0x3322, 0x09},
	{0x3342, 0x00},
	{0x3348, 0xE0},

	/* Shutter gain Settings */
	{0x0202, 0x04},
	{0x0203, 0x33},

	/* Streaming */
	{0x0100, 0x01},
	{IMX132_TABLE_WAIT_MS, IMX132_WAIT_MS},
	{IMX132_TABLE_END, 0x00}
};

enum {
	IMX132_MODE_1976X1200,
};

static struct imx132_reg *mode_table[] = {
	[IMX132_MODE_1976X1200] = mode_1976x1200,
};

static inline void
msleep_range(unsigned int delay_base)
{
	usleep_range(delay_base*1000, delay_base*1000+500);
}

static inline void
imx132_get_frame_length_regs(struct imx132_reg *regs, u32 frame_length)
{
	regs->addr = IMX132_FRAME_LEN_LINES_15_8;
	regs->val = (frame_length >> 8) & 0xff;
	(regs + 1)->addr = IMX132_FRAME_LEN_LINES_7_0;
	(regs + 1)->val = (frame_length) & 0xff;
}

static inline void
imx132_get_coarse_time_regs(struct imx132_reg *regs, u32 coarse_time)
{
	regs->addr = IMX132_COARSE_INTEGRATION_TIME_15_8;
	regs->val = (coarse_time >> 8) & 0xff;
	(regs + 1)->addr = IMX132_COARSE_INTEGRATION_TIME_7_0;
	(regs + 1)->val = (coarse_time) & 0xff;
}

static inline void
imx132_get_gain_reg(struct imx132_reg *regs, u16 gain)
{
	regs->addr = IMX132_ANA_GAIN_GLOBAL;
	regs->val = gain;
}

static int
imx132_read_reg(struct i2c_client *client, u16 addr, u8 *val)
{
	int err;
	struct i2c_msg msg[2];
	unsigned char data[3];

	if (!client->adapter)
		return -ENODEV;

	msg[0].addr = client->addr;
	msg[0].flags = 0;
	msg[0].len = 2;
	msg[0].buf = data;

	/* high byte goes out first */
	data[0] = (u8) (addr >> 8);
	data[1] = (u8) (addr & 0xff);

	msg[1].addr = client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = 1;
	msg[1].buf = data + 2;

	err = i2c_transfer(client->adapter, msg, 2);

	if (err != 2)
		return -EINVAL;

	*val = data[2];

	return 0;
}

static int
imx132_write_reg(struct i2c_client *client, u16 addr, u8 val)
{
	int err;
	struct i2c_msg msg;
	unsigned char data[3];

	if (!client->adapter)
		return -ENODEV;

	data[0] = (u8) (addr >> 8);
	data[1] = (u8) (addr & 0xff);
	data[2] = (u8) (val & 0xff);

	msg.addr = client->addr;
	msg.flags = 0;
	msg.len = 3;
	msg.buf = data;

	err = i2c_transfer(client->adapter, &msg, 1);
	if (err == 1)
		return 0;

	dev_err(&client->dev, "%s:i2c write failed, %x = %x\n",
			__func__, addr, val);

	return err;
}

static int
imx132_write_table(struct i2c_client *client,
			 const struct imx132_reg table[],
			 const struct imx132_reg override_list[],
			 int num_override_regs)
{
	int err;
	const struct imx132_reg *next;
	int i;
	u16 val;

	for (next = table; next->addr != IMX132_TABLE_END; next++) {
		if (next->addr == IMX132_TABLE_WAIT_MS) {
			msleep_range(next->val);
			continue;
		}

		val = next->val;

		/*
		 * When an override list is passed in, replace the reg
		 * value to write if the reg is in the list
		 */
		if (override_list) {
			for (i = 0; i < num_override_regs; i++) {
				if (next->addr == override_list[i].addr) {
					val = override_list[i].val;
					break;
				}
			}
		}

		err = imx132_write_reg(client, next->addr, val);
		if (err) {
			dev_err(&client->dev, "%s:imx132_write_table:%d",
				__func__, err);
			return err;
		}
	}
	return 0;
}

static int
imx132_set_mode(struct imx132_info *info, struct imx132_mode *mode)
{
	struct device *dev = &info->i2c_client->dev;
	int sensor_mode;
	int err;
	struct imx132_reg reg_list[5];

	dev_info(dev, "%s: res [%ux%u] framelen %u coarsetime %u gain %u\n",
		__func__, mode->xres, mode->yres,
		mode->frame_length, mode->coarse_time, mode->gain);

	if (mode->xres == 1976 && mode->yres == 1200)
		sensor_mode = IMX132_MODE_1976X1200;
	else {
		dev_err(dev, "%s: invalid resolution to set mode %d %d\n",
			__func__, mode->xres, mode->yres);
		return -EINVAL;
	}

	/*
	 * get a list of override regs for the asking frame length,
	 * coarse integration time, and gain.
	 */
	imx132_get_frame_length_regs(reg_list, mode->frame_length);
	imx132_get_coarse_time_regs(reg_list + 2, mode->coarse_time);
	imx132_get_gain_reg(reg_list + 4, mode->gain);

	err = imx132_write_table(info->i2c_client, mode_table[sensor_mode],
			reg_list, 5);
	if (err)
		return err;

	info->mode = sensor_mode;
	dev_info(dev, "[imx132]: stream on.\n");
	return 0;
}

static int
imx132_get_status(struct imx132_info *info, u8 *dev_status)
{
	/* TBD */
	*dev_status = 0;
	return 0;
}

static int
imx132_set_frame_length(struct imx132_info *info,
				u32 frame_length,
				bool group_hold)
{
	struct imx132_reg reg_list[2];
	int i = 0;
	int ret;

	imx132_get_frame_length_regs(reg_list, frame_length);

	if (group_hold) {
		ret = imx132_write_reg(info->i2c_client,
					IMX132_GROUP_PARAM_HOLD, 0x01);
		if (ret)
			return ret;
	}

	for (i = 0; i < NUM_OF_FRAME_LEN_REG; i++) {
		ret = imx132_write_reg(info->i2c_client, reg_list[i].addr,
			reg_list[i].val);
		if (ret)
			return ret;
	}

	if (group_hold) {
		ret = imx132_write_reg(info->i2c_client,
					IMX132_GROUP_PARAM_HOLD, 0x0);
		if (ret)
			return ret;
	}

	return 0;
}

static int
imx132_set_coarse_time(struct imx132_info *info,
				u32 coarse_time,
				bool group_hold)
{
	int ret;

	struct imx132_reg reg_list[2];
	int i = 0;

	imx132_get_coarse_time_regs(reg_list, coarse_time);

	if (group_hold) {
		ret = imx132_write_reg(info->i2c_client,
					IMX132_GROUP_PARAM_HOLD,
					0x01);
		if (ret)
			return ret;
	}

	for (i = 0; i < NUM_OF_COARSE_TIME_REG; i++) {
		ret = imx132_write_reg(info->i2c_client, reg_list[i].addr,
			reg_list[i].val);
		if (ret)
			return ret;
	}

	if (group_hold) {
		ret = imx132_write_reg(info->i2c_client,
					IMX132_GROUP_PARAM_HOLD, 0x0);
		if (ret)
			return ret;
	}
	return 0;
}

static int
imx132_set_gain(struct imx132_info *info, u16 gain, bool group_hold)
{
	int ret;
	struct imx132_reg reg_list;

	imx132_get_gain_reg(&reg_list, gain);

	if (group_hold) {
		ret = imx132_write_reg(info->i2c_client,
					IMX132_GROUP_PARAM_HOLD, 0x1);
		if (ret)
			return ret;
	}

	ret = imx132_write_reg(info->i2c_client, reg_list.addr, reg_list.val);
	if (ret)
		return ret;

	if (group_hold) {
		ret = imx132_write_reg(info->i2c_client,
					IMX132_GROUP_PARAM_HOLD, 0x0);
		if (ret)
			return ret;
	}
	return 0;
}

static int
imx132_set_group_hold(struct imx132_info *info, struct imx132_ae *ae)
{
	int ret;
	int count = 0;
	bool groupHoldEnabled = false;

	if (ae->gain_enable)
		count++;
	if (ae->coarse_time_enable)
		count++;
	if (ae->frame_length_enable)
		count++;
	if (count >= 2)
		groupHoldEnabled = true;

	if (groupHoldEnabled) {
		ret = imx132_write_reg(info->i2c_client,
					IMX132_GROUP_PARAM_HOLD, 0x1);
		if (ret)
			return ret;
	}

	if (ae->gain_enable)
		imx132_set_gain(info, ae->gain, false);
	if (ae->coarse_time_enable)
		imx132_set_coarse_time(info, ae->coarse_time, false);
	if (ae->frame_length_enable)
		imx132_set_frame_length(info, ae->frame_length, false);

	if (groupHoldEnabled) {
		ret = imx132_write_reg(info->i2c_client,
					IMX132_GROUP_PARAM_HOLD, 0x0);
		if (ret)
			return ret;
	}

	return 0;
}

static int imx132_get_sensor_id(struct imx132_info *info)
{
	int ret = 0;
	int i;
	u8 bak = 0;

	if (info->sensor_data.fuse_id_size)
		return 0;

	/*
	 * TBD 1: If the sensor does not have power at this point
	 * Need to supply the power, e.g. by calling power on function
	 */

	ret |= imx132_write_reg(info->i2c_client, 0x34C9, 0x10);
	for (i = 0; i < NUM_OF_SENSOR_ID_SPECIFIC_REG ; i++) {
		ret |= imx132_read_reg(info->i2c_client, 0x3580 + i, &bak);
		info->sensor_data.fuse_id[i] = bak;
	}

	if (!ret)
		info->sensor_data.fuse_id_size = i;

	return ret;
}

static long
imx132_ioctl(struct file *file,
			 unsigned int cmd, unsigned long arg)
{
	int err;
	struct imx132_info *info = file->private_data;
	struct device *dev = &info->i2c_client->dev;

	switch (cmd) {
	case IMX132_IOCTL_SET_MODE:
	{
		struct imx132_mode mode;
		if (copy_from_user(&mode,
			(const void __user *)arg,
			sizeof(struct imx132_mode))) {
			dev_err(dev, "%s:Failed to get mode from user.\n",
			__func__);
			return -EFAULT;
		}
		return imx132_set_mode(info, &mode);
	}
	case IMX132_IOCTL_SET_FRAME_LENGTH:
		return imx132_set_frame_length(info, (u32)arg, true);
	case IMX132_IOCTL_SET_COARSE_TIME:
		return imx132_set_coarse_time(info, (u32)arg, true);
	case IMX132_IOCTL_SET_GAIN:
		return imx132_set_gain(info, (u16)arg, true);
	case IMX132_IOCTL_GET_STATUS:
	{
		u8 status;

		err = imx132_get_status(info, &status);
		if (err)
			return err;
		if (copy_to_user((void __user *)arg, &status, 1)) {
			dev_err(dev, "%s:Failed to copy status to user.\n",
			__func__);
			return -EFAULT;
		}
		return 0;
	}
	case IMX132_IOCTL_GET_SENSORDATA:
	{
		err = imx132_get_sensor_id(info);

		if (err) {
			dev_err(dev, "%s:Failed to get fuse id info.\n",
			__func__);
			return err;
		}
		if (copy_to_user((void __user *)arg,
				&info->sensor_data,
				sizeof(struct imx132_sensordata))) {
			dev_info(dev, "%s:Fail copy fuse id to user space\n",
				__func__);
			return -EFAULT;
		}
		return 0;
	}
	case IMX132_IOCTL_SET_GROUP_HOLD:
	{
		struct imx132_ae ae;
		if (copy_from_user(&ae, (const void __user *)arg,
				sizeof(struct imx132_ae))) {
			dev_info(dev, "%s:fail group hold\n", __func__);
			return -EFAULT;
		}
		return imx132_set_group_hold(info, &ae);
	}
	default:
		dev_err(dev, "%s:unknown cmd.\n", __func__);
		return -EINVAL;
	}
	return 0;
}

static int
imx132_open(struct inode *inode, struct file *file)
{
	struct miscdevice	*miscdev = file->private_data;
	struct imx132_info	*info;

	info = container_of(miscdev, struct imx132_info, miscdev_info);
	/* check if the device is in use */
	if (atomic_xchg(&info->in_use, 1)) {
		dev_info(&info->i2c_client->dev, "%s:BUSY!\n", __func__);
		return -EBUSY;
	}

	file->private_data = info;

	if (info->pdata && info->pdata->power_on)
		info->pdata->power_on(&info->power);
	else {
		dev_err(&info->i2c_client->dev,
			"%s:no valid power_on function.\n", __func__);
		return -EEXIST;
	}

	return 0;
}

static int
imx132_release(struct inode *inode, struct file *file)
{
	struct imx132_info *info = file->private_data;

	if (info->pdata && info->pdata->power_off)
		info->pdata->power_off(&info->power);
	file->private_data = NULL;

	/* warn if device is already released */
	WARN_ON(!atomic_xchg(&info->in_use, 0));
	return 0;
}

static int imx132_power_put(struct imx132_power_rail *pw)
{
	if (likely(pw->dvdd))
		regulator_put(pw->dvdd);

	if (likely(pw->avdd))
		regulator_put(pw->avdd);

	if (likely(pw->iovdd))
		regulator_put(pw->iovdd);

	pw->dvdd = NULL;
	pw->avdd = NULL;
	pw->iovdd = NULL;

	return 0;
}

static int imx132_regulator_get(struct imx132_info *info,
	struct regulator **vreg, char vreg_name[])
{
	struct regulator *reg = NULL;
	int err = 0;

	reg = regulator_get(&info->i2c_client->dev, vreg_name);
	if (unlikely(IS_ERR(reg))) {
		dev_err(&info->i2c_client->dev, "%s %s ERR: %d\n",
			__func__, vreg_name, (int)reg);
		err = PTR_ERR(reg);
		reg = NULL;
	} else
		dev_dbg(&info->i2c_client->dev, "%s: %s\n",
			__func__, vreg_name);

	*vreg = reg;
	return err;
}

static int imx132_power_get(struct imx132_info *info)
{
	struct imx132_power_rail *pw = &info->power;

	imx132_regulator_get(info, &pw->dvdd, "vdig"); /* digital 1.2v */
	imx132_regulator_get(info, &pw->avdd, "vana"); /* analog 2.7v */
	imx132_regulator_get(info, &pw->iovdd, "vif"); /* interface 1.8v */

	return 0;
}

static const struct file_operations imx132_fileops = {
	.owner = THIS_MODULE,
	.open = imx132_open,
	.unlocked_ioctl = imx132_ioctl,
	.release = imx132_release,
};

static struct miscdevice imx132_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "imx132",
	.fops = &imx132_fileops,
};

static int
imx132_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct imx132_info *info;
	int err = 0;

	pr_info("[imx132]: probing sensor.\n");

	info = devm_kzalloc(&client->dev,
		sizeof(struct imx132_info), GFP_KERNEL);
	if (!info) {
		pr_err("[imx132]:%s:Unable to allocate memory!\n", __func__);
		return -ENOMEM;
	}

	info->pdata = client->dev.platform_data;
	info->i2c_client = client;
	atomic_set(&info->in_use, 0);
	info->mode = -1;

	i2c_set_clientdata(client, info);

	imx132_power_get(info);

	memcpy(&info->miscdev_info,
		&imx132_device,
		sizeof(struct miscdevice));

	err = misc_register(&info->miscdev_info);
	if (err) {
		imx132_power_put(&info->power);
		pr_err("[imx132]:%s:Unable to register misc device!\n",
		__func__);
	}

	return err;
}

static int
imx132_remove(struct i2c_client *client)
{
	struct imx132_info *info = i2c_get_clientdata(client);

	imx132_power_put(&info->power);
	misc_deregister(&imx132_device);
	return 0;
}

static const struct i2c_device_id imx132_id[] = {
	{ "imx132", 0 },
};

MODULE_DEVICE_TABLE(i2c, imx132_id);

static struct i2c_driver imx132_i2c_driver = {
	.driver = {
		.name = "imx132",
		.owner = THIS_MODULE,
	},
	.probe = imx132_probe,
	.remove = imx132_remove,
	.id_table = imx132_id,
};

static int __init
imx132_init(void)
{
	pr_info("[imx132] sensor driver loading\n");
	return i2c_add_driver(&imx132_i2c_driver);
}

static void __exit
imx132_exit(void)
{
	i2c_del_driver(&imx132_i2c_driver);
}

module_init(imx132_init);
module_exit(imx132_exit);
