/*
 * arch/arm/mach-tegra/tegra11_dvfs.c
 *
 * Copyright (C) 2012 NVIDIA Corporation.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/string.h>
#include <linux/module.h>
#include <linux/clk.h>
#include <linux/kobject.h>
#include <linux/err.h>

#include "clock.h"
#include "dvfs.h"
#include "fuse.h"
#include "board.h"
#include "tegra_cl_dvfs.h"

static bool tegra_dvfs_cpu_disabled;
static bool tegra_dvfs_core_disabled;

#define KHZ 1000
#define MHZ 1000000

/* FIXME: need tegra11 step */
#define VDD_SAFE_STEP			100

static int dvfs_temperatures[] = { 20, };

static struct tegra_cooling_device cpu_dfll_cdev = {
	.cdev_type = "cpu_dfll",
	.trip_temperatures = dvfs_temperatures,
	.trip_temperatures_num = ARRAY_SIZE(dvfs_temperatures),
};

static struct tegra_cooling_device cpu_pll_cdev = {
	.cdev_type = "cpu_pll",
	.trip_temperatures = dvfs_temperatures,
	.trip_temperatures_num = ARRAY_SIZE(dvfs_temperatures),
};

static struct tegra_cooling_device core_cdev = {
	.cdev_type = "core",
	.trip_temperatures = dvfs_temperatures,
	.trip_temperatures_num = ARRAY_SIZE(dvfs_temperatures),
};

static struct dvfs_rail tegra11_dvfs_rail_vdd_cpu = {
	.reg_id = "vdd_cpu",
	.max_millivolts = 1400,
	.min_millivolts = 800,
	.step = VDD_SAFE_STEP,
	.jmp_to_zero = true,
	.min_millivolts_cold = 1000,
	.dfll_mode_cdev = &cpu_dfll_cdev,
	.pll_mode_cdev = &cpu_pll_cdev,
};

static struct dvfs_rail tegra11_dvfs_rail_vdd_core = {
	.reg_id = "vdd_core",
	.max_millivolts = 1400,
	.min_millivolts = 800,
	.step = VDD_SAFE_STEP,
	.min_millivolts_cold = 950,
	.pll_mode_cdev = &core_cdev,
};

static struct dvfs_rail *tegra11_dvfs_rails[] = {
	&tegra11_dvfs_rail_vdd_cpu,
	&tegra11_dvfs_rail_vdd_core,
};

/* default cvb alignment on Tegra11 - 10mV */
int __attribute__((weak)) tegra_get_cvb_alignment_uV(void)
{
	return 10000;
}

/* CPU DVFS tables */
static struct cpu_cvb_dvfs cpu_cvb_dvfs_table[] = {
	{
		.speedo_id = 0,
		.process_id = -1,
		.dfll_tune_data  = {
			.tune0		= 0x00b0019d,
			.tune0_high_mv	= 0x00b0019d,
			.tune1		= 0x0000001f,
			.droop_rate_min = 1000000,
			.min_millivolts = 1000,
		},
		.max_mv = 1250,
		.freqs_mult = KHZ,
		.speedo_scale = 100,
		.voltage_scale = 100,
		.cvb_table = {
			/*f       dfll: c0,     c1,   c2  pll:  c0,   c1,    c2 */
			{ 306000, { 107330,  -1569,   0}, {  90000,    0,    0} },
			{ 408000, { 111250,  -1666,   0}, {  90000,    0,    0} },
			{ 510000, { 110000,  -1460,   0}, {  94000,    0,    0} },
			{ 612000, { 117290,  -1745,   0}, {  94000,    0,    0} },
			{ 714000, { 122700,  -1910,   0}, {  99000,    0,    0} },
			{ 816000, { 125620,  -1945,   0}, {  99000,    0,    0} },
			{ 918000, { 130560,  -2076,   0}, { 103000,    0,    0} },
			{1020000, { 137280,  -2303,   0}, { 103000,    0,    0} },
			{1122000, { 146440,  -2660,   0}, { 109000,    0,    0} },
			{1224000, { 152190,  -2825,   0}, { 109000,    0,    0} },
			{1326000, { 157520,  -2953,   0}, { 112000,    0,    0} },
			{1428000, { 166100,  -3261,   0}, { 140000,    0,    0} },
			{1530000, { 176410,  -3647,   0}, { 140000,    0,    0} },
			{1632000, { 189620,  -4186,   0}, { 140000,    0,    0} },
			{1734000, { 203190,  -4725,   0}, { 140000,    0,    0} },
			{1836000, { 222670,  -5573,   0}, { 140000,    0,    0} },
			{1938000, { 256210,  -7165,   0}, { 140000,    0,    0} },
			{2040000, { 250050,  -6544,   0}, { 140000,    0,    0} },
			{      0, {      0,      0,   0}, {      0,    0,    0} },
		},
	},
	{
		.speedo_id = 1,
		.process_id = 0,
		.dfll_tune_data  = {
			.tune0		= 0x00b0039d,
			.tune0_high_mv	= 0x00b0009d,
			.tune1		= 0x0000001f,
			.droop_rate_min = 1000000,
			.tune_high_min_millivolts = 1000,
			.min_millivolts = 900,
		},
		.max_mv = 1350,
		.freqs_mult = KHZ,
		.speedo_scale = 100,
		.voltage_scale = 1000,
		.cvb_table = {
			/*f       dfll:  c0,      c1,    c2  pll:   c0,   c1,    c2 */
			{ 306000, { 2190643, -141851, 3576}, {  900000,    0,    0} },
			{ 408000, { 2250968, -144331, 3576}, {  900000,    0,    0} },
			{ 510000, { 2313333, -146811, 3576}, {  940000,    0,    0} },
			{ 612000, { 2377738, -149291, 3576}, {  940000,    0,    0} },
			{ 714000, { 2444183, -151771, 3576}, {  990000,    0,    0} },
			{ 816000, { 2512669, -154251, 3576}, {  990000,    0,    0} },
			{ 918000, { 2583194, -156731, 3576}, { 1030000,    0,    0} },
			{1020000, { 2655759, -159211, 3576}, { 1030000,    0,    0} },
			{1122000, { 2730365, -161691, 3576}, { 1090000,    0,    0} },
			{1224000, { 2807010, -164171, 3576}, { 1090000,    0,    0} },
			{1326000, { 2885696, -166651, 3576}, { 1120000,    0,    0} },
			{1428000, { 2966422, -169131, 3576}, { 1400000,    0,    0} },
			{1530000, { 3049183, -171601, 3576}, { 1400000,    0,    0} },
			{1606500, { 3112179, -173451, 3576}, { 1400000,    0,    0} },
			{1708500, { 3198504, -175931, 3576}, { 1400000,    0,    0} },
			{1810500, { 3304747, -179126, 3576}, { 1400000,    0,    0} },
			{      0, {       0,       0,    0}, {       0,    0,    0} },
		},
	},
	{
		.speedo_id = 1,
		.process_id = 1,
		.dfll_tune_data  = {
			.tune0		= 0x00b0039d,
			.tune0_high_mv	= 0x00b0009d,
			.tune1		= 0x0000001f,
			.droop_rate_min = 1000000,
			.tune_high_min_millivolts = 1000,
			.min_millivolts = 900,
		},
		.max_mv = 1350,
		.freqs_mult = KHZ,
		.speedo_scale = 100,
		.voltage_scale = 1000,
		.cvb_table = {
			/*f       dfll:  c0,      c1,    c2  pll:   c0,   c1,    c2 */
			{ 306000, { 2190643, -141851, 3576}, {  900000,    0,    0} },
			{ 408000, { 2250968, -144331, 3576}, {  900000,    0,    0} },
			{ 510000, { 2313333, -146811, 3576}, {  940000,    0,    0} },
			{ 612000, { 2377738, -149291, 3576}, {  940000,    0,    0} },
			{ 714000, { 2444183, -151771, 3576}, {  990000,    0,    0} },
			{ 816000, { 2512669, -154251, 3576}, {  990000,    0,    0} },
			{ 918000, { 2583194, -156731, 3576}, { 1030000,    0,    0} },
			{1020000, { 2655759, -159211, 3576}, { 1030000,    0,    0} },
			{1122000, { 2730365, -161691, 3576}, { 1090000,    0,    0} },
			{1224000, { 2807010, -164171, 3576}, { 1090000,    0,    0} },
			{1326000, { 2885696, -166651, 3576}, { 1120000,    0,    0} },
			{1428000, { 2966422, -169131, 3576}, { 1400000,    0,    0} },
			{1530000, { 3049183, -171601, 3576}, { 1400000,    0,    0} },
			{1606500, { 3112179, -173451, 3576}, { 1400000,    0,    0} },
			{1708500, { 3198504, -175931, 3576}, { 1400000,    0,    0} },
			{1810500, { 3304747, -179126, 3576}, { 1400000,    0,    0} },
			{      0, {       0,       0,    0}, {       0,    0,    0} },
		},
	},
	{
		.speedo_id = 2,
		.process_id = -1,
		.dfll_tune_data  = {
			.tune0		= 0x00b0039d,
			.tune0_high_mv	= 0x00b0009d,
			.tune1		= 0x0000001f,
			.droop_rate_min = 1000000,
			.tune_high_min_millivolts = 1000,
			.min_millivolts = 900,
		},
		.max_mv = 1350,
		.freqs_mult = KHZ,
		.speedo_scale = 100,
		.voltage_scale = 1000,
		.cvb_table = {
			/*f       dfll:  c0,      c1,    c2  pll:   c0,   c1,    c2 */
			{ 306000, { 2190643, -141851, 3576}, {  900000,    0,    0} },
			{ 408000, { 2250968, -144331, 3576}, {  900000,    0,    0} },
			{ 510000, { 2313333, -146811, 3576}, {  940000,    0,    0} },
			{ 612000, { 2377738, -149291, 3576}, {  940000,    0,    0} },
			{ 714000, { 2444183, -151771, 3576}, {  990000,    0,    0} },
			{ 816000, { 2512669, -154251, 3576}, {  990000,    0,    0} },
			{ 918000, { 2583194, -156731, 3576}, { 1030000,    0,    0} },
			{1020000, { 2655759, -159211, 3576}, { 1030000,    0,    0} },
			{1122000, { 2730365, -161691, 3576}, { 1090000,    0,    0} },
			{1224000, { 2807010, -164171, 3576}, { 1090000,    0,    0} },
			{1326000, { 2885696, -166651, 3576}, { 1120000,    0,    0} },
			{1428000, { 2966422, -169131, 3576}, { 1400000,    0,    0} },
			{1530000, { 3049183, -171601, 3576}, { 1400000,    0,    0} },
			{1606500, { 3112179, -173451, 3576}, { 1400000,    0,    0} },
			{1708500, { 3198504, -175931, 3576}, { 1400000,    0,    0} },
			{1810500, { 3304747, -179126, 3576}, { 1400000,    0,    0} },
			{1912500, { 3395401, -181606, 3576}, { 1400000,    0,    0} },
			{      0, {       0,       0,    0}, {       0,    0,    0} },
		},
	},
};

static int cpu_millivolts[MAX_DVFS_FREQS];
static int cpu_dfll_millivolts[MAX_DVFS_FREQS];

static struct dvfs cpu_dvfs = {
	.clk_name	= "cpu_g",
	.millivolts	= cpu_millivolts,
	.dfll_millivolts = cpu_dfll_millivolts,
	.auto_dvfs	= true,
	.dvfs_rail	= &tegra11_dvfs_rail_vdd_cpu,
};

/* Core DVFS tables */
/* FIXME: real data */
static const int core_millivolts[MAX_DVFS_FREQS] = {
	900, 950, 1000, 1050, 1100, 1120, 1170, 1200, 1250};

#define CORE_DVFS(_clk_name, _speedo_id, _process_id, _auto, _mult, _freqs...) \
	{							\
		.clk_name	= _clk_name,			\
		.speedo_id	= _speedo_id,			\
		.process_id	= _process_id,			\
		.freqs		= {_freqs},			\
		.freqs_mult	= _mult,			\
		.millivolts	= core_millivolts,		\
		.auto_dvfs	= _auto,			\
		.dvfs_rail	= &tegra11_dvfs_rail_vdd_core,	\
	}

static struct dvfs core_dvfs_table[] = {
	/* Core voltages (mV):		         900,    950,   1000,   1050,    1100,    1120,    1170,    1200,    1250 */
	/* Clock limits for internal blocks, PLLs */
#ifndef CONFIG_TEGRA_SIMULATION_PLATFORM
	CORE_DVFS("emc",    -1, -1, 1, KHZ,        1,      1,      1,      1,  800000,  800000,  933000,  933000, 1066000),

	CORE_DVFS("cpu_lp",  0,  0, 1, KHZ,   228000, 306000, 396000, 528000,  648000,  696000,  696000,  696000,  696000),
	CORE_DVFS("cpu_lp",  0,  1, 1, KHZ,   324000, 432000, 528000, 612000,  696000,  696000,  696000,  696000,  696000),
	CORE_DVFS("cpu_lp",  1,  1, 1, KHZ,   324000, 432000, 528000, 612000,  792000,  816000,  816000,  816000,  816000),

	CORE_DVFS("sbus",    0,  0, 1, KHZ,   132000, 188000, 240000, 276000,  324000,  336000,  336000,  336000,  336000),
	CORE_DVFS("sbus",    0,  1, 1, KHZ,   216000, 264000, 300000, 336000,  336000,  336000,  336000,  336000,  336000),
	CORE_DVFS("sbus",    1,  1, 1, KHZ,   216000, 264000, 300000, 336000,  372000,  384000,  384000,  384000,  384000),

	CORE_DVFS("vi",     -1,  0, 1, KHZ,   144000, 216000, 240000, 312000,  372000,  408000,  408000,  408000,  408000),
	CORE_DVFS("vi",     -1,  1, 1, KHZ,   144000, 216000, 240000, 408000,  408000,  408000,  408000,  408000,  408000),

	CORE_DVFS("2d",     -1,  0, 1, KHZ,   192000, 228000, 300000, 396000,  492000,  516000,  552000,  552000,  600000),
	CORE_DVFS("3d",     -1,  0, 1, KHZ,   192000, 228000, 300000, 396000,  492000,  516000,  552000,  552000,  600000),
	CORE_DVFS("epp",    -1,  0, 1, KHZ,   192000, 228000, 300000, 396000,  492000,  516000,  552000,  552000,  600000),

	CORE_DVFS("2d",     -1,  1, 1, KHZ,   276000, 348000, 420000, 492000,  528000,  564000,  600000,  636000,  672000),
	CORE_DVFS("3d",     -1,  1, 1, KHZ,   276000, 348000, 420000, 492000,  528000,  564000,  600000,  636000,  672000),
	CORE_DVFS("epp",    -1,  1, 1, KHZ,   276000, 348000, 420000, 492000,  528000,  564000,  600000,  636000,  672000),

	CORE_DVFS("msenc",   0,  0, 1, KHZ,   144000, 182000, 240000, 312000,  384000,  408000,  408000,  408000,  408000),
	CORE_DVFS("se",      0,  0, 1, KHZ,   144000, 182000, 240000, 312000,  384000,  408000,  408000,  408000,  408000),
	CORE_DVFS("tsec",    0,  0, 1, KHZ,   144000, 182000, 240000, 312000,  384000,  408000,  408000,  408000,  408000),
	CORE_DVFS("vde",     0,  0, 1, KHZ,   144000, 182000, 240000, 312000,  384000,  408000,  408000,  408000,  408000),

	CORE_DVFS("msenc",   0,  1, 1, KHZ,   228000, 288000, 360000, 408000,  408000,  408000,  408000,  408000,  408000),
	CORE_DVFS("se",      0,  1, 1, KHZ,   228000, 288000, 360000, 408000,  408000,  408000,  408000,  408000,  408000),
	CORE_DVFS("tsec",    0,  1, 1, KHZ,   228000, 288000, 360000, 408000,  408000,  408000,  408000,  408000,  408000),
	CORE_DVFS("vde",     0,  1, 1, KHZ,   228000, 288000, 360000, 408000,  408000,  408000,  408000,  408000,  408000),

	CORE_DVFS("msenc",   1,  1, 1, KHZ,   228000, 288000, 360000, 420000,  468000,  480000,  480000,  480000,  480000),
	CORE_DVFS("se",      1,  1, 1, KHZ,   228000, 288000, 360000, 420000,  468000,  480000,  480000,  480000,  480000),
	CORE_DVFS("tsec",    1,  1, 1, KHZ,   228000, 288000, 360000, 420000,  468000,  480000,  480000,  480000,  480000),
	CORE_DVFS("vde",     1,  1, 1, KHZ,   228000, 288000, 360000, 420000,  468000,  480000,  480000,  480000,  480000),

	CORE_DVFS("host1x",  0,  0, 1, KHZ,   144000, 188000, 240000, 276000,  324000,  336000,  336000,  336000,  336000),
	CORE_DVFS("host1x",  0,  1, 1, KHZ,   216000, 264000, 300000, 336000,  336000,  336000,  336000,  336000,  336000),
	CORE_DVFS("host1x",  1,  1, 1, KHZ,   216000, 264000, 300000, 336000,  372000,  384000,  384000,  384000,  384000),

#ifdef CONFIG_TEGRA_DUAL_CBUS
	CORE_DVFS("c2bus",  -1,  0, 1, KHZ,   192000, 228000, 300000, 396000,  492000,  516000,  552000,  552000,  600000),
	CORE_DVFS("c2bus",  -1,  1, 1, KHZ,   276000, 348000, 420000, 492000,  528000,  564000,  600000,  636000,  672000),
	CORE_DVFS("c3bus",   0,  0, 1, KHZ,   144000, 182000, 240000, 312000,  384000,  408000,  408000,  408000,  408000),
	CORE_DVFS("c3bus",   0,  1, 1, KHZ,   228000, 288000, 360000, 408000,  408000,  408000,  408000,  408000,  408000),
	CORE_DVFS("c3bus",   1,  1, 1, KHZ,   228000, 288000, 360000, 420000,  468000,  480000,  480000,  480000,  480000),
#else
	CORE_DVFS("cbus",    0,  0, 1, KHZ,   144000, 182000, 240000, 312000,  384000,  408000,  408000,  408000,  408000),
	CORE_DVFS("cbus",    0,  1, 1, KHZ,   228000, 288000, 360000, 408000,  408000,  408000,  408000,  408000,  408000),
	CORE_DVFS("cbus",    1,  1, 1, KHZ,   228000, 288000, 360000, 420000,  468000,  480000,  480000,  480000,  480000),
#endif

	CORE_DVFS("pll_m",  -1, -1, 1, KHZ,   800000, 800000, 1066000, 1066000, 1066000, 1066000, 1066000, 1066000, 1066000),
	CORE_DVFS("pll_c",  -1, -1, 1, KHZ,   800000, 800000, 1066000, 1066000, 1066000, 1066000, 1066000, 1066000, 1066000),
	CORE_DVFS("pll_c2", -1, -1, 1, KHZ,   800000, 800000, 1066000, 1066000, 1066000, 1066000, 1066000, 1066000, 1066000),
	CORE_DVFS("pll_c3", -1, -1, 1, KHZ,   800000, 800000, 1066000, 1066000, 1066000, 1066000, 1066000, 1066000, 1066000),

	/* Core voltages (mV):		         900,    950,   1000,   1050,    1100,    1120,    1170,    1200,    1250 */
	/* Clock limits for I/O peripherals */
	CORE_DVFS("sbc1",   -1, -1, 1, KHZ,    48000,  48000,  48000,  48000,   52000,   52000,   52000,   52000,   52000),
	CORE_DVFS("sbc2",   -1, -1, 1, KHZ,    48000,  48000,  48000,  48000,   52000,   52000,   52000,   52000,   52000),
	CORE_DVFS("sbc3",   -1, -1, 1, KHZ,    48000,  48000,  48000,  48000,   52000,   52000,   52000,   52000,   52000),
	CORE_DVFS("sbc4",   -1, -1, 1, KHZ,    48000,  48000,  48000,  48000,   52000,   52000,   52000,   52000,   52000),
	CORE_DVFS("sbc5",   -1, -1, 1, KHZ,    48000,  48000,  48000,  48000,   52000,   52000,   52000,   52000,   52000),
	CORE_DVFS("sbc6",   -1, -1, 1, KHZ,    48000,  48000,  48000,  48000,   52000,   52000,   52000,   52000,   52000),

	CORE_DVFS("sdmmc1", -1, -1, 1, KHZ,        1,  81600,  81600,  81600,  156000,  156000,  156000,  156000,  156000),
	CORE_DVFS("sdmmc3", -1, -1, 1, KHZ,        1,  81600,  81600,  81600,  156000,  156000,  156000,  156000,  156000),
	CORE_DVFS("sdmmc4", -1, -1, 1, KHZ,   102000, 102000, 102000, 102000,  102000,  102000,  198000,  198000,  198000),

	CORE_DVFS("csi",    -1, -1, 1, KHZ,        1, 102000, 102000, 102000,  102000,  102000,  102000,  102000,  102000),
	CORE_DVFS("cilab",  -1, -1, 1, KHZ,        1, 102000, 102000, 102000,  102000,  102000,  102000,  102000,  102000),
	CORE_DVFS("cilcd",  -1, -1, 1, KHZ,        1, 102000, 102000, 102000,  102000,  102000,  102000,  102000,  102000),
	CORE_DVFS("cile",   -1, -1, 1, KHZ,        1, 102000, 102000, 102000,  102000,  102000,  102000,  102000,  102000),

	CORE_DVFS("dsia",   -1, -1, 1, KHZ,   500000, 500000, 500000, 500000,  500000,  500000,  500000,  500000,  500000),
	CORE_DVFS("dsib",   -1, -1, 1, KHZ,   500000, 500000, 500000, 500000,  500000,  500000,  500000,  500000,  500000),
	CORE_DVFS("dsialp", -1, -1, 1, KHZ,        1, 102000, 102000, 102000,  102000,  102000,  102000,  102000,  102000),
	CORE_DVFS("dsiblp", -1, -1, 1, KHZ,        1, 102000, 102000, 102000,  102000,  102000,  102000,  102000,  102000),
	CORE_DVFS("hdmi",   -1, -1, 1, KHZ,   148500, 148500, 148500, 297000,  297000,  297000,  297000,  297000,  297000),

	/*
	 * The clock rate for the display controllers that determines the
	 * necessary core voltage depends on a divider that is internal
	 * to the display block.  Disable auto-dvfs on the display clocks,
	 * and let the display driver call tegra_dvfs_set_rate manually
	 */
	CORE_DVFS("disp1",  -1, -1, 0, KHZ,   166000, 166000, 166000, 297000,  297000,  297000,  297000,  297000,  297000),
	CORE_DVFS("disp2",  -1, -1, 0, KHZ,   166000, 166000, 166000, 297000,  297000,  297000,  297000,  297000,  297000),

	/* xusb clocks */
	CORE_DVFS("xusb_falcon_src", -1, -1, 1, KHZ,  1, 336000, 336000, 336000,  336000,  336000,  336000,  336000,  336000),
	CORE_DVFS("xusb_host_src",   -1, -1, 1, KHZ,  1, 112000, 112000, 112000,  112000,  112000,  112000,  112000,  112000),
	CORE_DVFS("xusb_dev_src",    -1, -1, 1, KHZ,  1,  58300,  58300, 112000,  112000,  112000,  112000,  112000,  112000),
	CORE_DVFS("xusb_ss_src",     -1, -1, 1, KHZ,  1, 120000, 120000, 120000,  120000,  120000,  120000,  120000,  120000),
	CORE_DVFS("xusb_fs_src",     -1, -1, 1, KHZ,  1,  48000,  48000,  48000,   48000,   48000,   48000,   48000,   48000),
	CORE_DVFS("xusb_hs_src",     -1, -1, 1, KHZ,  1,  60000,  60000,  60000,   60000,   60000,   60000,   60000,   60000),
#endif
};

int tegra_dvfs_disable_core_set(const char *arg, const struct kernel_param *kp)
{
	int ret;

	ret = param_set_bool(arg, kp);
	if (ret)
		return ret;

	if (tegra_dvfs_core_disabled)
		tegra_dvfs_rail_disable(&tegra11_dvfs_rail_vdd_core);
	else
		tegra_dvfs_rail_enable(&tegra11_dvfs_rail_vdd_core);

	return 0;
}

int tegra_dvfs_disable_cpu_set(const char *arg, const struct kernel_param *kp)
{
	int ret;

	ret = param_set_bool(arg, kp);
	if (ret)
		return ret;

	if (tegra_dvfs_cpu_disabled)
		tegra_dvfs_rail_disable(&tegra11_dvfs_rail_vdd_cpu);
	else
		tegra_dvfs_rail_enable(&tegra11_dvfs_rail_vdd_cpu);

	return 0;
}

int tegra_dvfs_disable_get(char *buffer, const struct kernel_param *kp)
{
	return param_get_bool(buffer, kp);
}

static struct kernel_param_ops tegra_dvfs_disable_core_ops = {
	.set = tegra_dvfs_disable_core_set,
	.get = tegra_dvfs_disable_get,
};

static struct kernel_param_ops tegra_dvfs_disable_cpu_ops = {
	.set = tegra_dvfs_disable_cpu_set,
	.get = tegra_dvfs_disable_get,
};

module_param_cb(disable_core, &tegra_dvfs_disable_core_ops,
	&tegra_dvfs_core_disabled, 0644);
module_param_cb(disable_cpu, &tegra_dvfs_disable_cpu_ops,
	&tegra_dvfs_cpu_disabled, 0644);


static bool __init can_update_max_rate(struct clk *c, struct dvfs *d)
{
	/* Don't update manual dvfs clocks */
	if (!d->auto_dvfs)
		return false;

	/*
	 * Don't update EMC shared bus, since EMC dvfs is board dependent: max
	 * rate and EMC scaling frequencies are determined by tegra BCT (flashed
	 * together with the image) and board specific EMC DFS table; we will
	 * check the scaling ladder against nominal core voltage when the table
	 * is loaded (and if on particular board the table is not loaded, EMC
	 * scaling is disabled).
	 */
	if (c->ops->shared_bus_update && (c->flags & PERIPH_EMC_ENB))
		return false;

	/*
	 * Don't update shared cbus, and don't propagate common cbus dvfs
	 * limit down to shared users, but set maximum rate for each user
	 * equal to the respective client limit.
	 */
	if (c->ops->shared_bus_update && (c->flags & PERIPH_ON_CBUS)) {
		struct clk *user;
		unsigned long rate;

		list_for_each_entry(
			user, &c->shared_bus_list, u.shared_bus_user.node) {
			if (user->u.shared_bus_user.client) {
				rate = user->u.shared_bus_user.client->max_rate;
				user->max_rate = rate;
				user->u.shared_bus_user.rate = rate;
			}
		}
		return false;
	}

	/* Other, than EMC and cbus, auto-dvfs clocks can be updated */
	return true;
}

static void __init init_dvfs_one(struct dvfs *d, int max_freq_index)
{
	int ret;
	struct clk *c = tegra_get_clock_by_name(d->clk_name);

	if (!c) {
		pr_debug("tegra11_dvfs: no clock found for %s\n",
			d->clk_name);
		return;
	}

	/* Update max rate for auto-dvfs clocks, with shared bus exceptions */
	if (can_update_max_rate(c, d)) {
		BUG_ON(!d->freqs[max_freq_index]);
		tegra_init_max_rate(
			c, d->freqs[max_freq_index] * d->freqs_mult);
	}
	d->max_millivolts = d->dvfs_rail->nominal_millivolts;

	ret = tegra_enable_dvfs_on_clk(c, d);
	if (ret)
		pr_err("tegra11_dvfs: failed to enable dvfs on %s\n", c->name);
}

static bool __init match_dvfs_one(struct dvfs *d, int speedo_id, int process_id)
{
	if ((d->process_id != -1 && d->process_id != process_id) ||
		(d->speedo_id != -1 && d->speedo_id != speedo_id)) {
		pr_debug("tegra11_dvfs: rejected %s speedo %d,"
			" process %d\n", d->clk_name, d->speedo_id,
			d->process_id);
		return false;
	}
	return true;
}

static bool __init match_cpu_cvb_one(struct cpu_cvb_dvfs *d,
				     int speedo_id, int process_id)
{
	if ((d->process_id != -1 && d->process_id != process_id) ||
		(d->speedo_id != -1 && d->speedo_id != speedo_id)) {
		pr_debug("tegra11_dvfs: rejected cpu cvb speedo %d,"
			" process %d\n", d->speedo_id, d->process_id);
		return false;
	}
	return true;
}

/* cvb_mv = ((c2 * speedo / s_scale + c1) * speedo / s_scale + c0) / v_scale */
static inline int get_cvb_voltage(int speedo, int s_scale,
				  struct cpu_cvb_dvfs_parameters *cvb)
{
	/* apply only speedo scale: output mv = cvb_mv * v_scale */
	int mv;
	mv = DIV_ROUND_CLOSEST(cvb->c2 * speedo, s_scale);
	mv = DIV_ROUND_CLOSEST((mv + cvb->c1) * speedo, s_scale) + cvb->c0;
	return mv;
}

static inline int round_cvb_voltage(int mv, int v_scale)
{
	/* combined: apply voltage scale and round to cvb alignment step */
	int cvb_align_step_uv = tegra_get_cvb_alignment_uV();

	return DIV_ROUND_UP(mv * 1000, v_scale * cvb_align_step_uv) *
		cvb_align_step_uv / 1000;
}

static int __init set_cpu_dvfs_data(
	struct cpu_cvb_dvfs *d, struct dvfs *cpu_dvfs, int *max_freq_index)
{
	int i, j, mv, dfll_mv, min_dfll_mv;
	unsigned long fmax_at_vmin = 0;
	unsigned long fmax_pll_mode = 0;
	unsigned long fmin_use_dfll = 0;
	struct cpu_cvb_dvfs_table *table = NULL;
	int speedo = tegra_cpu_speedo_value();

	min_dfll_mv = d->dfll_tune_data.min_millivolts;
	BUG_ON(min_dfll_mv < tegra11_dvfs_rail_vdd_cpu.min_millivolts);

	/*
	 * Use CVB table to fill in CPU dvfs frequencies and voltages. Each
	 * CVB entry specifies CPU frequency and CVB coefficients to calculate
	 * the respective voltage when either DFLL or PLL is used as CPU clock
	 * source.
	 *
	 * Minimum voltage limit is applied only to DFLL source. For PLL source
	 * voltage can go as low as table specifies. Maximum voltage limit is
	 * applied to both sources, but differently: directly clip voltage for
	 * DFLL, and limit maximum frequency for PLL.
	 */
	for (i = 0, j = 0; i < MAX_DVFS_FREQS; i++) {
		table = &d->cvb_table[i];
		if (!table->freq)
			break;

		dfll_mv = get_cvb_voltage(
			speedo, d->speedo_scale, &table->cvb_dfll_param);
		dfll_mv = round_cvb_voltage(dfll_mv, d->voltage_scale);

		mv = get_cvb_voltage(
			speedo, d->speedo_scale, &table->cvb_pll_param);
		mv = round_cvb_voltage(mv, d->voltage_scale);

		/* Check maximum frequency at minimum voltage for dfll source */
		dfll_mv = max(dfll_mv, min_dfll_mv);
		if (dfll_mv > min_dfll_mv) {
			if (!j)
				break;	/* 1st entry already above Vmin */
			if (!fmax_at_vmin)
				fmax_at_vmin = cpu_dvfs->freqs[j - 1];
		}

		/* Clip maximum frequency at maximum voltage for pll source */
		if (mv > d->max_mv) {
			if (!j)
				break;	/* 1st entry already above Vmax */
			if (!fmax_pll_mode)
				fmax_pll_mode = cpu_dvfs->freqs[j - 1];
		}

		/* Minimum rate with pll source voltage above dfll Vmin */
		if ((mv >= min_dfll_mv) && (!fmin_use_dfll))
			fmin_use_dfll = table->freq;

		/* fill in dvfs tables */
		cpu_dvfs->freqs[j] = table->freq;
		cpu_dfll_millivolts[j] = min(dfll_mv, d->max_mv);
		cpu_millivolts[j] = mv;
		j++;

		/*
		 * "Round-up" frequency list cut-off (keep first entry that
		 *  exceeds max voltage - the voltage limit will be enforced
		 *  anyway, so when requested this frequency dfll will settle
		 *  at whatever high frequency it can on the particular chip)
		 */
		if (dfll_mv > d->max_mv)
			break;
	}
	/* Table must not be empty and must have and at least one entry below,
	   and one entry above Vmin */
	if (!i || !j || !fmax_at_vmin) {
		pr_err("tegra11_dvfs: invalid cpu dvfs table\n");
		return -ENOENT;
	}

	/* Must have crossover between dfll and pll operating ranges */
	if (!fmin_use_dfll || (fmin_use_dfll > fmax_at_vmin)) {
		pr_err("tegra11_dvfs: no crossover of dfll and pll voltages\n");
		return -EINVAL;
	}

	/* dvfs tables are successfully populated - fill in the rest */
	cpu_dvfs->speedo_id = d->speedo_id;
	cpu_dvfs->process_id = d->process_id;
	cpu_dvfs->freqs_mult = d->freqs_mult;
	cpu_dvfs->dvfs_rail->nominal_millivolts = min(d->max_mv,
		max(cpu_millivolts[j - 1], cpu_dfll_millivolts[j - 1]));
	*max_freq_index = j - 1;

	cpu_dvfs->dfll_data = d->dfll_tune_data;
	cpu_dvfs->dfll_data.max_rate_boost = fmax_pll_mode ?
		(cpu_dvfs->freqs[j - 1] - fmax_pll_mode) * d->freqs_mult : 0;
	cpu_dvfs->dfll_data.out_rate_min = fmax_at_vmin * d->freqs_mult;
	cpu_dvfs->dfll_data.use_dfll_rate_min = fmin_use_dfll * d->freqs_mult;
	cpu_dvfs->dfll_data.min_millivolts = min_dfll_mv;
	return 0;
}

static int __init get_core_nominal_mv_index(int speedo_id)
{
	int i;
	int mv = tegra_core_speedo_mv();
	int core_edp_voltage = get_core_edp();

	/*
	 * Start with nominal level for the chips with this speedo_id. Then,
	 * make sure core nominal voltage is below edp limit for the board
	 * (if edp limit is set).
	 */
	if (!core_edp_voltage)
		core_edp_voltage = 1100;	/* default 1.1V EDP limit */

	mv = min(mv, core_edp_voltage);

	/* Round nominal level down to the nearest core scaling step */
	for (i = 0; i < MAX_DVFS_FREQS; i++) {
		if ((core_millivolts[i] == 0) || (mv < core_millivolts[i]))
			break;
	}

	if (i == 0) {
		pr_err("tegra11_dvfs: unable to adjust core dvfs table to"
		       " nominal voltage %d\n", mv);
		return -ENOSYS;
	}
	return i - 1;
}

int tegra_cpu_dvfs_alter(int edp_thermal_index, const cpumask_t *cpus,
			 bool before_clk_update, int cpu_event)
{
	/* empty definition for tegra11 */
	return 0;
}

void __init tegra11x_init_dvfs(void)
{
	int cpu_speedo_id = tegra_cpu_speedo_id();
	int cpu_process_id = tegra_cpu_process_id();
	int soc_speedo_id = tegra_soc_speedo_id();
	int core_process_id = tegra_core_process_id();

	int i, ret;
	int core_nominal_mv_index;
	int cpu_max_freq_index = 0;

#ifndef CONFIG_TEGRA_CORE_DVFS
	tegra_dvfs_core_disabled = true;
#endif
#ifndef CONFIG_TEGRA_CPU_DVFS
	tegra_dvfs_cpu_disabled = true;
#endif
	/* Setup rail bins */
	tegra11_dvfs_rail_vdd_cpu.stats.bin_uV = tegra_get_cvb_alignment_uV();
	tegra11_dvfs_rail_vdd_core.stats.bin_uV = tegra_get_cvb_alignment_uV();

	/*
	 * Find nominal voltages for core (1st) and cpu rails before rail
	 * init. Nominal voltage index in core scaling ladder can also be
	 * used to determine max dvfs frequencies for all core clocks. In
	 * case of error disable core scaling and set index to 0, so that
	 * core clocks would not exceed rates allowed at minimum voltage.
	 */
	core_nominal_mv_index = get_core_nominal_mv_index(soc_speedo_id);
	if (core_nominal_mv_index < 0) {
		tegra11_dvfs_rail_vdd_core.disabled = true;
		tegra_dvfs_core_disabled = true;
		core_nominal_mv_index = 0;
	}
	tegra11_dvfs_rail_vdd_core.nominal_millivolts =
		core_millivolts[core_nominal_mv_index];

	/*
	 * Setup cpu dvfs and dfll tables from cvb data, determine nominal
	 * voltage for cpu rail, and cpu maximum frequency. Note that entire
	 * frequency range is guaranteed only when dfll is used as cpu clock
	 * source. Reaching maximum frequency with pll as cpu clock source
	 * may not be possible within nominal voltage range (dvfs mechanism
	 * would automatically fail frequency request in this case, so that
	 * voltage limit is not violated). Error when cpu dvfs table can not
	 * be constructed must never happen.
	 */
	for (ret = 0, i = 0; i <  ARRAY_SIZE(cpu_cvb_dvfs_table); i++) {
		struct cpu_cvb_dvfs *d = &cpu_cvb_dvfs_table[i];
		if (match_cpu_cvb_one(d, cpu_speedo_id, cpu_process_id)) {
			ret = set_cpu_dvfs_data(
				d, &cpu_dvfs, &cpu_max_freq_index);
			break;
		}
	}
	BUG_ON((i == ARRAY_SIZE(cpu_cvb_dvfs_table)) || ret);

	/* Init rail structures and dependencies */
	tegra_dvfs_init_rails(tegra11_dvfs_rails,
		ARRAY_SIZE(tegra11_dvfs_rails));

	/* Search core dvfs table for speedo/process matching entries and
	   initialize dvfs-ed clocks */
	for (i = 0; i <  ARRAY_SIZE(core_dvfs_table); i++) {
		struct dvfs *d = &core_dvfs_table[i];
		if (!match_dvfs_one(d, soc_speedo_id, core_process_id))
			continue;
		init_dvfs_one(d, core_nominal_mv_index);
	}

	/* Initialize matching cpu dvfs entry already found when nominal
	   voltage was determined */
	init_dvfs_one(&cpu_dvfs, cpu_max_freq_index);

	/* Finally disable dvfs on rails if necessary */
	if (tegra_dvfs_core_disabled)
		tegra_dvfs_rail_disable(&tegra11_dvfs_rail_vdd_core);
	if (tegra_dvfs_cpu_disabled)
		tegra_dvfs_rail_disable(&tegra11_dvfs_rail_vdd_cpu);

	pr_info("tegra dvfs: VDD_CPU nominal %dmV, scaling %s\n",
		tegra11_dvfs_rail_vdd_cpu.nominal_millivolts,
		tegra_dvfs_cpu_disabled ? "disabled" : "enabled");
	pr_info("tegra dvfs: VDD_CORE nominal %dmV, scaling %s\n",
		tegra11_dvfs_rail_vdd_core.nominal_millivolts,
		tegra_dvfs_core_disabled ? "disabled" : "enabled");
}

int tegra_dvfs_rail_disable_prepare(struct dvfs_rail *rail)
{
	return 0;
}

int tegra_dvfs_rail_post_enable(struct dvfs_rail *rail)
{
	return 0;
}

/*
 * sysfs and dvfs interfaces to cap tegra core domains frequencies
 */
static DEFINE_MUTEX(core_cap_lock);

struct core_cap {
	int refcnt;
	int level;
};
static struct core_cap core_buses_cap;
static struct core_cap kdvfs_core_cap;
static struct core_cap user_core_cap;

static struct kobject *cap_kobj;

/* Arranged in order required for enabling/lowering the cap */
static struct {
	const char *cap_name;
	struct clk *cap_clk;
	unsigned long freqs[MAX_DVFS_FREQS];
} core_cap_table[] = {
#ifdef CONFIG_TEGRA_DUAL_CBUS
	{ .cap_name = "cap.c2bus" },
	{ .cap_name = "cap.c3bus" },
#else
	{ .cap_name = "cap.cbus" },
#endif
	{ .cap_name = "cap.sclk" },
	{ .cap_name = "cap.emc" },
};


static void core_cap_level_set(int level)
{
	int i, j;

	for (j = 0; j < ARRAY_SIZE(core_millivolts); j++) {
		int v = core_millivolts[j];
		if ((v == 0) || (level < v))
			break;
	}
	j = (j == 0) ? 0 : j - 1;
	level = core_millivolts[j];

	if (level < core_buses_cap.level) {
		for (i = 0; i < ARRAY_SIZE(core_cap_table); i++)
			if (core_cap_table[i].cap_clk)
				clk_set_rate(core_cap_table[i].cap_clk,
					     core_cap_table[i].freqs[j]);
	} else if (level > core_buses_cap.level) {
		for (i = ARRAY_SIZE(core_cap_table) - 1; i >= 0; i--)
			if (core_cap_table[i].cap_clk)
				clk_set_rate(core_cap_table[i].cap_clk,
					     core_cap_table[i].freqs[j]);
	}
	core_buses_cap.level = level;
}

static void core_cap_update(void)
{
	int new_level = tegra11_dvfs_rail_vdd_core.max_millivolts;

	if (kdvfs_core_cap.refcnt)
		new_level = min(new_level, kdvfs_core_cap.level);
	if (user_core_cap.refcnt)
		new_level = min(new_level, user_core_cap.level);

	if (core_buses_cap.level != new_level)
		core_cap_level_set(new_level);
}

static void core_cap_enable(bool enable)
{
	if (enable)
		core_buses_cap.refcnt++;
	else if (core_buses_cap.refcnt)
		core_buses_cap.refcnt--;

	core_cap_update();
}

static ssize_t
core_cap_state_show(struct kobject *kobj, struct kobj_attribute *attr,
		    char *buf)
{
	return sprintf(buf, "%d (%d)\n", core_buses_cap.refcnt ? 1 : 0,
			user_core_cap.refcnt ? 1 : 0);
}
static ssize_t
core_cap_state_store(struct kobject *kobj, struct kobj_attribute *attr,
		     const char *buf, size_t count)
{
	int state;

	if (sscanf(buf, "%d", &state) != 1)
		return -1;

	mutex_lock(&core_cap_lock);

	if (state) {
		user_core_cap.refcnt++;
		if (user_core_cap.refcnt == 1)
			core_cap_enable(true);
	} else if (user_core_cap.refcnt) {
		user_core_cap.refcnt--;
		if (user_core_cap.refcnt == 0)
			core_cap_enable(false);
	}

	mutex_unlock(&core_cap_lock);
	return count;
}

static ssize_t
core_cap_level_show(struct kobject *kobj, struct kobj_attribute *attr,
		    char *buf)
{
	return sprintf(buf, "%d (%d)\n", core_buses_cap.level,
			user_core_cap.level);
}
static ssize_t
core_cap_level_store(struct kobject *kobj, struct kobj_attribute *attr,
		     const char *buf, size_t count)
{
	int level;

	if (sscanf(buf, "%d", &level) != 1)
		return -1;

	mutex_lock(&core_cap_lock);
	user_core_cap.level = level;
	core_cap_update();
	mutex_unlock(&core_cap_lock);
	return count;
}

static struct kobj_attribute cap_state_attribute =
	__ATTR(core_cap_state, 0644, core_cap_state_show, core_cap_state_store);
static struct kobj_attribute cap_level_attribute =
	__ATTR(core_cap_level, 0644, core_cap_level_show, core_cap_level_store);

const struct attribute *cap_attributes[] = {
	&cap_state_attribute.attr,
	&cap_level_attribute.attr,
	NULL,
};

void tegra_dvfs_core_cap_enable(bool enable)
{
	mutex_lock(&core_cap_lock);

	if (enable) {
		kdvfs_core_cap.refcnt++;
		if (kdvfs_core_cap.refcnt == 1)
			core_cap_enable(true);
	} else if (kdvfs_core_cap.refcnt) {
		kdvfs_core_cap.refcnt--;
		if (kdvfs_core_cap.refcnt == 0)
			core_cap_enable(false);
	}
	mutex_unlock(&core_cap_lock);
}

void tegra_dvfs_core_cap_level_set(int level)
{
	mutex_lock(&core_cap_lock);
	kdvfs_core_cap.level = level;
	core_cap_update();
	mutex_unlock(&core_cap_lock);
}

static int __init init_core_cap_one(struct clk *c, unsigned long *freqs)
{
	int i, v, next_v = 0;
	unsigned long rate, next_rate = 0;

	for (i = 0; i < ARRAY_SIZE(core_millivolts); i++) {
		v = core_millivolts[i];
		if (v == 0)
			break;

		for (;;) {
			rate = next_rate;
			next_rate = clk_round_rate(c->parent, rate + 1000);
			if (IS_ERR_VALUE(next_rate)) {
				pr_debug("tegra11_dvfs: failed to round %s rate %lu\n",
					 c->name, rate);
				return -EINVAL;
			}
			if (rate == next_rate)
				break;

			next_v = tegra_dvfs_predict_millivolts(
				c->parent, next_rate);
			if (IS_ERR_VALUE(next_v)) {
				pr_debug("tegra11_dvfs: failed to predict %s mV for rate %lu\n",
					 c->name, next_rate);
				return -EINVAL;
			}
			if (next_v > v)
				break;
		}

		if (rate == 0) {
			rate = next_rate;
			pr_warn("tegra11_dvfs: minimum %s rate %lu requires %d mV\n",
				c->name, rate, next_v);
		}
		freqs[i] = rate;
		next_rate = rate;
	}
	return 0;
}

static int __init tegra_dvfs_init_core_cap(void)
{
	int i;
	struct clk *c = NULL;

	core_buses_cap.level = kdvfs_core_cap.level = user_core_cap.level =
		tegra11_dvfs_rail_vdd_core.max_millivolts;

	for (i = 0; i < ARRAY_SIZE(core_cap_table); i++) {
		c = tegra_get_clock_by_name(core_cap_table[i].cap_name);
		if (!c || !c->parent ||
		    init_core_cap_one(c, core_cap_table[i].freqs)) {
			pr_err("tegra11_dvfs: failed to initialize %s table\n",
			       core_cap_table[i].cap_name);
			continue;
		}
		core_cap_table[i].cap_clk = c;
	}

	cap_kobj = kobject_create_and_add("tegra_cap", kernel_kobj);
	if (!cap_kobj) {
		pr_err("tegra11_dvfs: failed to create sysfs cap object\n");
		return 0;
	}

	if (sysfs_create_files(cap_kobj, cap_attributes)) {
		pr_err("tegra11_dvfs: failed to create sysfs cap interface\n");
		return 0;
	}
	pr_info("tegra dvfs: tegra sysfs cap interface is initialized\n");

	return 0;
}
late_initcall(tegra_dvfs_init_core_cap);
