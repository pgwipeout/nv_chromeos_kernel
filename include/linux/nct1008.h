/*
 * include/linux/nct1008.h
 *
 * NCT1008, temperature monitoring device from ON Semiconductors
 *
 * Copyright (c) 2010-2012, NVIDIA Corporation.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef _LINUX_NCT1008_H
#define _LINUX_NCT1008_H

#include <linux/types.h>
#include <linux/workqueue.h>
#include <linux/thermal.h>

struct nct1008_data;

enum nct1008_chip { NCT1008, NCT72 };


struct nct_trip_temp {
	char *cdev_type;
	long trip_temp;
	enum thermal_trip_type trip_type;
	unsigned long state;
	long hysteresis;
	bool is_enabled;
};

#define NCT_MAX_TRIPS (32)

struct nct1008_platform_data {
	bool supported_hwrev;
	bool ext_range;
	u8 conv_rate;
	u8 offset;
	s16 shutdown_ext_limit;
	s16 shutdown_local_limit;

	int passive_delay;

	int num_trips;
	struct nct_trip_temp trips[NCT_MAX_TRIPS];
	const char *loc_name;
};
#endif /* _LINUX_NCT1008_H */
