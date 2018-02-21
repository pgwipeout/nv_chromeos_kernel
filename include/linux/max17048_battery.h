/*
 * Copyright (C) 2009 Samsung Electronics
 * Copyright (C) 2012 Nvidia Cooperation
 * Minkyu Kang <mk7.kang@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __MAX17048_BATTERY_H_
#define __MAX17048_BATTERY_H_
#include <linux/smb349-charger.h>

#define MAX17048_DATA_SIZE 64

struct max17048_battery_model {
	uint8_t rcomp;
	uint8_t soccheck_A;
	uint8_t soccheck_B;
	uint8_t bits;
	uint8_t alert_threshold;
	uint8_t one_percent_alerts;
	uint8_t alert_on_reset;
	uint16_t rcomp_seg;
	uint16_t hibernate;
	uint16_t vreset;
	uint16_t valert;
	uint16_t ocvtest;
	uint8_t data_tbl[MAX17048_DATA_SIZE];
};

struct max17048_platform_data {
	int use_ac;
	int use_usb;
	struct max17048_battery_model *model_data;
};

void max17048_battery_status(int status, int chrg_type);
int max17048_check_battery(void);
#endif
