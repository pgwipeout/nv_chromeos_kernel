/*
 * arch/arm/mach-tegra/tegra_fuse.c
 *
 * Copyright (C) 2010 Google, Inc.
 * Copyright (C) 2010-2012 NVIDIA Corp.
 *
 * Author:
 *	Colin Cross <ccross@android.com>
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
#include <linux/io.h>
#include <linux/init.h>
#include <linux/string.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/export.h>

#include <mach/iomap.h>
#include <mach/tegra_fuse.h>
#include <mach/hardware.h>
#include <mach/gpufuse.h>

#include "fuse.h"
#include "apbio.h"

#define FUSE_SKU_INFO		0x110
#define FUSE_VP8_ENABLE_0	0x1c4
#if defined(CONFIG_ARCH_TEGRA_2x_SOC)
#define FUSE_UID_LOW		0x108
#define FUSE_UID_HIGH		0x10c
#define FUSE_SPARE_BIT		0x200
#else
#define FUSE_VENDOR_CODE	0x200
#define FUSE_VENDOR_CODE_MASK	0xf
#define FUSE_FAB_CODE		0x204
#define FUSE_FAB_CODE_MASK	0x3f
#define FUSE_LOT_CODE_0		0x208
#define FUSE_LOT_CODE_1		0x20c
#define FUSE_WAFER_ID		0x210
#define FUSE_WAFER_ID_MASK	0x3f
#define FUSE_X_COORDINATE	0x214
#define FUSE_X_COORDINATE_MASK	0x1ff
#define FUSE_Y_COORDINATE	0x218
#define FUSE_Y_COORDINATE_MASK	0x1ff
#define FUSE_GPU_INFO		0x390
#define FUSE_GPU_INFO_MASK	(1<<2)
#if defined(CONFIG_ARCH_TEGRA_3x_SOC)
#define FUSE_SPARE_BIT		0x244
#endif
#if defined(CONFIG_ARCH_TEGRA_11x_SOC)
#define FUSE_SPARE_BIT		0x280
#endif
/* fuse registers used in public fuse data read API */
#define FUSE_TEST_PROGRAM_REVISION_0	0x128
/* fuse spare bits are used to get Tj-ADT values */
#define FUSE_SPARE_BIT_0_0	0x244
#define NUM_TSENSOR_SPARE_BITS	28
/* tsensor calibration register */
#define FUSE_TSENSOR_CALIB_0	0x198

#if defined(CONFIG_ARCH_TEGRA_11x_SOC)
#define FUSE_VSENSOR_CALIB_0	0x18c
#endif

#endif

#define TEGRA_AGE_0_6 0x2cc /*Spare bit 34*/
#define TEGRA_AGE_1_6 0x308 /*Spare bit 49*/
#define TEGRA_AGE_0_5 0x2c8 /*Spare bit 33*/
#define TEGRA_AGE_1_5 0x304 /*Spare bit 48*/
#define TEGRA_AGE_0_4 0x2c4 /*Spare bit 32*/
#define TEGRA_AGE_1_4 0x300 /*Spare bit 47*/
#define TEGRA_AGE_0_3 0x2c0 /*Spare bit 31*/
#define TEGRA_AGE_1_3 0x2fc /*Spare bit 46*/
#define TEGRA_AGE_0_2 0x2bc /*Spare bit 30*/
#define TEGRA_AGE_1_2 0x2f8 /*Spare bit 45*/
#define TEGRA_AGE_0_1 0x2b8 /*Spare bit 29*/
#define TEGRA_AGE_1_1 0x2f4 /*Spare bit 44*/
#define TEGRA_AGE_0_0 0x2b4 /*Spare bit 28*/
#define TEGRA_AGE_1_0 0x2f0 /*Spare bit 43*/

struct tegra_id {
	enum tegra_chipid chipid;
	unsigned int major, minor, netlist, patch;
	enum tegra_revision revision;
	char *priv;
};

static struct tegra_id tegra_id;

int tegra_sku_id;
int tegra_chip_id;
enum tegra_revision tegra_revision;
static unsigned int tegra_fuse_vp8_enable;
static int tegra_gpu_num_pixel_pipes;
static int tegra_gpu_num_alus_per_pixel_pipe;

/* The BCT to use at boot is specified by board straps that can be read
 * through a APB misc register and decoded. 2 bits, i.e. 4 possible BCTs.
 */
int tegra_bct_strapping;

#define STRAP_OPT 0x008
#define GMI_AD0 (1 << 4)
#define GMI_AD1 (1 << 5)
#define RAM_ID_MASK (GMI_AD0 | GMI_AD1)
#define RAM_CODE_SHIFT 4

static const char *tegra_revision_name[TEGRA_REVISION_MAX] = {
	[TEGRA_REVISION_UNKNOWN] = "unknown",
	[TEGRA_REVISION_A01]     = "A01",
	[TEGRA_REVISION_A02]     = "A02",
	[TEGRA_REVISION_A03]     = "A03",
	[TEGRA_REVISION_A03p]    = "A03 prime",
	[TEGRA_REVISION_A04]     = "A04",
	[TEGRA_REVISION_A04p]    = "A04 prime",
	[TEGRA_REVISION_QT]      = "QT",
};

u32 tegra_fuse_readl(unsigned long offset)
{
	return tegra_apb_readl(TEGRA_FUSE_BASE + offset);
}

void tegra_fuse_writel(u32 val, unsigned long offset)
{
	tegra_apb_writel(val, TEGRA_FUSE_BASE + offset);
}

static inline bool get_spare_fuse(int bit)
{
	return tegra_fuse_readl(FUSE_SPARE_BIT + bit * 4);
}

const char *tegra_get_revision_name(void)
{
	return tegra_revision_name[tegra_revision];
}

#ifdef CONFIG_ARCH_TEGRA_2x_SOC
int tegra_fuse_get_revision(u32 *rev)
{
	return -ENOENT;
}
EXPORT_SYMBOL(tegra_fuse_get_revision);

int tegra_fuse_get_tsensor_calibration_data(u32 *calib)
{
	return -ENOENT;
}
EXPORT_SYMBOL(tegra_fuse_get_tsensor_calibration_data);

int tegra_fuse_get_tsensor_spare_bits(u32 *spare_bits)
{
	return -ENOENT;
}
EXPORT_SYMBOL(tegra_fuse_get_tsensor_spare_bits);

#else

#if defined(CONFIG_ARCH_TEGRA_11x_SOC)
int tegra_fuse_get_vsensor_calib(u32 *calib)
{
	*calib = tegra_fuse_readl(FUSE_VSENSOR_CALIB_0);
	return 0;
}

static int tsensor_calib_offset[] = {
	[0] = 0x198,
	[1] = 0x184,
	[2] = 0x188,
	[3] = 0x22c,
	[4] = 0x254,
	[5] = 0x258,
	[6] = 0x25c,
	[7] = 0x260,
};

int tegra_fuse_get_tsensor_calib(int index, u32 *calib)
{
	if (index < 0 || index > 7)
		return -EINVAL;
	*calib = tegra_fuse_readl(tsensor_calib_offset[index]);
	return 0;
}
#endif

int tegra_fuse_get_revision(u32 *rev)
{
	/* fuse revision */
	*rev = tegra_fuse_readl(FUSE_TEST_PROGRAM_REVISION_0);
	return 0;
}
EXPORT_SYMBOL(tegra_fuse_get_revision);

int tegra_fuse_get_tsensor_calibration_data(u32 *calib)
{
	/* tsensor calibration fuse */
	*calib = tegra_fuse_readl(FUSE_TSENSOR_CALIB_0);
	return 0;
}
EXPORT_SYMBOL(tegra_fuse_get_tsensor_calibration_data);

int tegra_fuse_get_tsensor_spare_bits(u32 *spare_bits)
{
	u32 value;
	int i;

	BUG_ON(NUM_TSENSOR_SPARE_BITS > (sizeof(u32) * 8));
	if (!spare_bits)
		return -ENOMEM;
	*spare_bits = 0;
	/* spare bits 0-27 */
	for (i = 0; i < NUM_TSENSOR_SPARE_BITS; i++) {
		value = tegra_fuse_readl(FUSE_SPARE_BIT_0_0 +
			(i << 2));
		if (value)
			*spare_bits |= BIT(i);
	}
	return 0;
}
EXPORT_SYMBOL(tegra_fuse_get_tsensor_spare_bits);
#endif

#define TEGRA_READ_AGE_BIT(n, bit, age) {\
	bit = tegra_fuse_readl(TEGRA_AGE_0_##n);\
	bit |= tegra_fuse_readl(TEGRA_AGE_1_##n);\
	bit = bit << n;\
	age |= bit;\
}

int tegra_get_age(void)
{
	int linear_age, age_bit;
	linear_age = age_bit = 0;

	TEGRA_READ_AGE_BIT(6, age_bit, linear_age);
	TEGRA_READ_AGE_BIT(5, age_bit, linear_age);
	TEGRA_READ_AGE_BIT(4, age_bit, linear_age);
	TEGRA_READ_AGE_BIT(3, age_bit, linear_age);
	TEGRA_READ_AGE_BIT(2, age_bit, linear_age);
	TEGRA_READ_AGE_BIT(1, age_bit, linear_age);
	TEGRA_READ_AGE_BIT(0, age_bit, linear_age);

	/*Default Aug, 2012*/
	if (linear_age <= 0)
		linear_age = 8;

	pr_info("TEGRA: Linear age: %d\n", linear_age);

	return linear_age;
}

unsigned long long tegra_chip_uid(void)
{
#if defined(CONFIG_ARCH_TEGRA_2x_SOC)
	unsigned long long lo, hi;

	lo = tegra_fuse_readl(FUSE_UID_LOW);
	hi = tegra_fuse_readl(FUSE_UID_HIGH);
	return (hi << 32ull) | lo;
#else
	u64 uid = 0ull;
	u32 reg;
	u32 cid;
	u32 vendor;
	u32 fab;
	u32 lot;
	u32 wafer;
	u32 x;
	u32 y;
	u32 i;

	/* This used to be so much easier in prior chips. Unfortunately, there
	   is no one-stop shopping for the unique id anymore. It must be
	   constructed from various bits of information burned into the fuses
	   during the manufacturing process. The 64-bit unique id is formed
	   by concatenating several bit fields. The notation used for the
	   various fields is <fieldname:size_in_bits> with the UID composed
	   thusly:

	   <CID:4><VENDOR:4><FAB:6><LOT:26><WAFER:6><X:9><Y:9>

	   Where:

		Field    Bits  Position Data
		-------  ----  -------- ----------------------------------------
		CID        4     60     Chip id
		VENDOR     4     56     Vendor code
		FAB        6     50     FAB code
		LOT       26     24     Lot code (5-digit base-36-coded-decimal,
					re-encoded to 26 bits binary)
		WAFER      6     18     Wafer id
		X          9      9     Wafer X-coordinate
		Y          9      0     Wafer Y-coordinate
		-------  ----
		Total     64
	*/

	/* Get the chip id and encode each chip variant as a unique value. */
	reg = readl(IO_TO_VIRT(TEGRA_APB_MISC_BASE + 0x804));
	reg = (reg & 0xFF00) >> 8;

	switch (reg) {
	case TEGRA_CHIPID_TEGRA3:
		cid = 0;
		break;

	case TEGRA_CHIPID_TEGRA11:
		cid = 1;
		break;

	default:
		BUG();
		break;
	}

	vendor = tegra_fuse_readl(FUSE_VENDOR_CODE) & FUSE_VENDOR_CODE_MASK;
	fab = tegra_fuse_readl(FUSE_FAB_CODE) & FUSE_FAB_CODE_MASK;

	/* Lot code must be re-encoded from a 5 digit base-36 'BCD' number
	   to a binary number. */
	lot = 0;
	reg = tegra_fuse_readl(FUSE_LOT_CODE_0) << 2;

	for (i = 0; i < 5; ++i) {
		u32 digit = (reg & 0xFC000000) >> 26;
		BUG_ON(digit >= 36);
		lot *= 36;
		lot += digit;
		reg <<= 6;
	}

	wafer = tegra_fuse_readl(FUSE_WAFER_ID) & FUSE_WAFER_ID_MASK;
	x = tegra_fuse_readl(FUSE_X_COORDINATE) & FUSE_X_COORDINATE_MASK;
	y = tegra_fuse_readl(FUSE_Y_COORDINATE) & FUSE_Y_COORDINATE_MASK;

	uid = ((unsigned long long)cid  << 60ull)
	    | ((unsigned long long)vendor << 56ull)
	    | ((unsigned long long)fab << 50ull)
	    | ((unsigned long long)lot << 24ull)
	    | ((unsigned long long)wafer << 18ull)
	    | ((unsigned long long)x << 9ull)
	    | ((unsigned long long)y << 0ull);
	return uid;
#endif
}

unsigned int tegra_spare_fuse(int bit)
{
	BUG_ON(bit < 0 || bit > 61);
	return tegra_fuse_readl(FUSE_SPARE_BIT + bit * 4);
}

int tegra_gpu_register_sets(void)
{
#ifdef CONFIG_ARCH_TEGRA_HAS_DUAL_3D
	u32 reg = readl(IO_TO_VIRT(TEGRA_CLK_RESET_BASE + FUSE_GPU_INFO));
	if (reg & FUSE_GPU_INFO_MASK)
		return 1;
	else
		return 2;
#else
	return 1;
#endif
}

void tegra_gpu_get_info(struct gpu_info *pInfo)
{
	if (tegra_get_chipid() == TEGRA_CHIPID_TEGRA11) {
		pInfo->num_pixel_pipes = 4;
		pInfo->num_alus_per_pixel_pipe = 3;
	} else {
		pInfo->num_pixel_pipes = 1;
		pInfo->num_alus_per_pixel_pipe = 1;
	}
}

static int get_gpu_num_pixel_pipes(char *val, const struct kernel_param *kp)
{
	struct gpu_info gpu_info;

	tegra_gpu_get_info(&gpu_info);
	tegra_gpu_num_pixel_pipes = gpu_info.num_pixel_pipes;
	return param_get_uint(val, kp);
}

static int get_gpu_num_alus_per_pixel_pipe(char *val,
						const struct kernel_param *kp)
{
	struct gpu_info gpu_info;

	tegra_gpu_get_info(&gpu_info);
	tegra_gpu_num_alus_per_pixel_pipe = gpu_info.num_alus_per_pixel_pipe;

	return param_get_uint(val, kp);
}

static struct kernel_param_ops tegra_gpu_num_pixel_pipes_ops = {
	.get = get_gpu_num_pixel_pipes,
};

static struct kernel_param_ops tegra_gpu_num_alus_per_pixel_pipe_ops = {
	.get = get_gpu_num_alus_per_pixel_pipe,
};

module_param_cb(tegra_gpu_num_pixel_pipes, &tegra_gpu_num_pixel_pipes_ops,
		&tegra_gpu_num_pixel_pipes, 0444);
module_param_cb(tegra_gpu_num_alus_per_pixel_pipe,
		&tegra_gpu_num_alus_per_pixel_pipe_ops,
		&tegra_gpu_num_alus_per_pixel_pipe, 0444);

#if defined(CONFIG_TEGRA_SILICON_PLATFORM)
struct chip_revision {
	enum tegra_chipid	chipid;
	unsigned int		major;
	unsigned int		minor;
	char			prime;
	enum tegra_revision	revision;
};

#define CHIP_REVISION(id, m, n, p, rev) {	\
	.chipid = TEGRA_CHIPID_##id,		\
	.major = m,				\
	.minor = n,				\
	.prime = p,				\
	.revision = TEGRA_REVISION_##rev }

static struct chip_revision tegra_chip_revisions[] = {
	CHIP_REVISION(TEGRA2,  1, 2, 0,   A02),
	CHIP_REVISION(TEGRA2,  1, 3, 0,   A03),
	CHIP_REVISION(TEGRA2,  1, 3, 'p', A03p),
	CHIP_REVISION(TEGRA2,  1, 4, 0,   A04),
	CHIP_REVISION(TEGRA2,  1, 4, 'p', A04p),
	CHIP_REVISION(TEGRA3,  1, 1, 0,   A01),
	CHIP_REVISION(TEGRA3,  1, 2, 0,   A02),
	CHIP_REVISION(TEGRA3,  1, 3, 0,   A03),
	CHIP_REVISION(TEGRA11, 1, 1, 0,   A01),
};
#endif

static enum tegra_revision tegra_decode_revision(const struct tegra_id *id)
{
	enum tegra_revision revision = TEGRA_REVISION_UNKNOWN;

#if defined(CONFIG_TEGRA_SILICON_PLATFORM)
	int i ;
	char prime;

	if (id->priv == NULL)
		prime = 0;
	else
		prime = *(id->priv);

	for (i = 0; i < ARRAY_SIZE(tegra_chip_revisions); i++) {
		if ((id->chipid != tegra_chip_revisions[i].chipid) ||
		    (id->minor != tegra_chip_revisions[i].minor) ||
		    (id->major != tegra_chip_revisions[i].major) ||
		    (prime != tegra_chip_revisions[i].prime))
			continue;

		revision = tegra_chip_revisions[i].revision;
		break;
	}

#elif defined(CONFIG_TEGRA_FPGA_PLATFORM)
	if (id->major == 0) {
		if (id->minor == 1)
			revision = TEGRA_REVISION_A01;
		else
			revision = TEGRA_REVISION_QT;
	}
#elif defined(CONFIG_TEGRA_SIMULATION_PLATFORM)
	if ((id->chipid & 0xff) == TEGRA_CHIPID_TEGRA11)
		revision = TEGRA_REVISION_A01;
#endif

	return revision;
}

static void tegra_set_tegraid(u32 chipid,
					u32 major, u32 minor,
					u32 nlist, u32 patch, const char *priv)
{
	tegra_id.chipid  = (enum tegra_chipid) chipid;
	tegra_id.major   = major;
	tegra_id.minor   = minor;
	tegra_id.netlist = nlist;
	tegra_id.patch   = patch;
	tegra_id.priv    = (char *)priv;
	tegra_id.revision = tegra_decode_revision(&tegra_id);
}

static void tegra_get_tegraid_from_hw(void)
{
	void __iomem *chip_id = IO_ADDRESS(TEGRA_APB_MISC_BASE) + 0x804;
	void __iomem *netlist = IO_ADDRESS(TEGRA_APB_MISC_BASE) + 0x860;
	u32 cid = readl(chip_id);
	u32 nlist = readl(netlist);
	char *priv = NULL;

#ifdef CONFIG_ARCH_TEGRA_2x_SOC
	if (get_spare_fuse(18) || get_spare_fuse(19))
		priv = "p";
#endif
	tegra_set_tegraid((cid >> 8) & 0xff,
			  (cid >> 4) & 0xf,
			  (cid >> 16) & 0xf,
			  (nlist >> 0) & 0xffff,
			  (nlist >> 16) & 0xffff,
			  priv);
}

enum tegra_chipid tegra_get_chipid(void)
{
	if (tegra_id.chipid == TEGRA_CHIPID_UNKNOWN)
		tegra_get_tegraid_from_hw();

	return tegra_id.chipid;
}

enum tegra_revision tegra_get_revision(void)
{
	if (tegra_id.chipid == TEGRA_CHIPID_UNKNOWN)
		tegra_get_tegraid_from_hw();

	return tegra_id.revision;
}

#ifndef CONFIG_TEGRA_SILICON_PLATFORM
void tegra_get_netlist_revision(u32 *netlist, u32* patchid)
{
	if (tegra_id.chipid == TEGRA_CHIPID_UNKNOWN) {
		/* Boot loader did not pass a valid chip ID.
		 * Get it from hardware */
		tegra_get_tegraid_from_hw();
	}
	*netlist = tegra_id.netlist;
	*patchid = tegra_id.patch & 0xF;
}
#endif

static int get_chip_id(char *val, const struct kernel_param *kp)
{
	return param_get_uint(val, kp);
}

static int get_revision(char *val, const struct kernel_param *kp)
{
	return param_get_uint(val, kp);
}

static struct kernel_param_ops tegra_chip_id_ops = {
	.get = get_chip_id,
};

static struct kernel_param_ops tegra_revision_ops = {
	.get = get_revision,
};

module_param_cb(tegra_chip_id, &tegra_chip_id_ops, &tegra_id.chipid, 0444);
module_param_cb(tegra_chip_rev, &tegra_revision_ops, &tegra_id.revision, 0444);

void tegra_init_fuse(void)
{
	u32 id;

	u32 reg = readl(IO_TO_VIRT(TEGRA_CLK_RESET_BASE + 0x48));
	reg |= 1 << 28;
	writel(reg, IO_TO_VIRT(TEGRA_CLK_RESET_BASE + 0x48));

	reg = tegra_fuse_readl(FUSE_SKU_INFO);
	tegra_sku_id = reg & 0xFF;

	reg = tegra_apb_readl(TEGRA_APB_MISC_BASE + STRAP_OPT);
	tegra_bct_strapping = (reg & RAM_ID_MASK) >> RAM_CODE_SHIFT;

	id = readl_relaxed(IO_ADDRESS(TEGRA_APB_MISC_BASE) + 0x804);
	tegra_chip_id = (id >> 8) & 0xff;

	tegra_revision = tegra_get_revision();

	tegra_init_speedo_data();

	pr_info("Tegra Revision: %s SKU: 0x%x CPU Process: %d Core Process: %d\n",
		tegra_revision_name[tegra_revision],
		tegra_sku_id, tegra_cpu_process_id(),
		tegra_core_process_id());
}

static unsigned int get_fuse_vp8_enable(char *val, struct kernel_param *kp)
{
	if (tegra_get_chipid() == TEGRA_CHIPID_TEGRA2 ||
		 tegra_get_chipid() == TEGRA_CHIPID_TEGRA3)
		tegra_fuse_vp8_enable = 0;
	else
		tegra_fuse_vp8_enable =  tegra_fuse_readl(FUSE_VP8_ENABLE_0);

	return param_get_uint(val, kp);
}

module_param_call(tegra_fuse_vp8_enable, NULL, get_fuse_vp8_enable,
		&tegra_fuse_vp8_enable, 0444);
__MODULE_PARM_TYPE(tegra_fuse_vp8_enable, "uint");
