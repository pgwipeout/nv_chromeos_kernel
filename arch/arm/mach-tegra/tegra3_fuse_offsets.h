/*
 * copyright (c) 2012, nvidia corporation.
 *
 * this program is free software; you can redistribute it and/or modify
 * it under the terms of the gnu general public license as published by
 * the free software foundation; either version 2 of the license, or
 * (at your option) any later version.
 *
 * this program is distributed in the hope that it will be useful, but without
 * any warranty; without even the implied warranty of merchantability or
 * fitness for a particular purpose.  see the gnu general public license for
 * more details.
 *
 * you should have received a copy of the gnu general public license along
 * with this program; if not, write to the free software foundation, inc.,
 * 51 franklin street, fifth floor, boston, ma  02110-1301, usa.
 */

#ifndef __TEGRA3_FUSE_OFFSETS_H
#define __TEGRA3_FUSE_OFFSETS_H

/* private_key4 */
#define DEVKEY_START_OFFSET 0x16
#define DEVKEY_START_BIT    22

/* arm_debug_dis */
#define JTAG_START_OFFSET 0x0
#define JTAG_START_BIT    24

/* security_mode */
#define ODM_PROD_START_OFFSET 0x0
#define ODM_PROD_START_BIT    23

/* boot_device_info */
#define SB_DEVCFG_START_OFFSET 0x18
#define SB_DEVCFG_START_BIT    22

/* reserved_sw[2:0] */
#define SB_DEVSEL_START_OFFSET 0x1A
#define SB_DEVSEL_START_BIT    6

/* private_key0 -> private_key3 */
#define SBK_START_OFFSET 0x0E
#define SBK_START_BIT    22

/* reserved_sw[7:4] */
#define SW_RESERVED_START_OFFSET 0x1A
#define SW_RESERVED_START_BIT    10

/* reserved_sw[3] */
#define IGNORE_DEVSEL_START_OFFSET 0x1A
#define IGNORE_DEVSEL_START_BIT    9

/* reserved_odm0 -> reserved_odm7 */
#define ODM_RESERVED_DEVSEL_START_OFFSET 0x1A
#define ODM_RESERVED_START_BIT    14

#endif /* __TEGRA3_FUSE_OFFSETS_H */
