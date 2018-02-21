/*
 * arch/arm/mach-tegra/board-roth.c
 *
 * Copyright (c) 2012, NVIDIA CORPORATION.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
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

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/ctype.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/serial_8250.h>
#include <linux/i2c.h>
#include <linux/dma-mapping.h>
#include <linux/delay.h>
#include <linux/i2c-tegra.h>
#include <linux/gpio.h>
#include <linux/input.h>
#include <linux/platform_data/tegra_usb.h>
#include <linux/tegra_uart.h>
#include <linux/memblock.h>
#include <linux/rfkill-gpio.h>
#include <linux/skbuff.h>
#include <linux/ti_wilink_st.h>
#include <linux/regulator/consumer.h>
#include <linux/max17048_battery.h>
#include <linux/leds.h>
#include <linux/leds_pwm.h>
#include <linux/i2c/at24.h>
#include <linux/of_platform.h>
#include <asm/hardware/gic.h>

#include <mach/clk.h>
#include <mach/iomap.h>
#include <mach/irqs.h>
#include <mach/pinmux.h>
#include <mach/pinmux-tegra30.h>
#include <mach/iomap.h>
#include <mach/io.h>
#include <mach/io_dpd.h>
#include <mach/i2s.h>
#include <mach/tegra_asoc_pdata.h>
#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <mach/usb_phy.h>
#include <mach/gpio-tegra.h>
#include <mach/tegra_fiq_debugger.h>
#include <mach/edp.h>

#include "board.h"
#include "board-common.h"
#include "clock.h"
#include "board-roth.h"
#include "devices.h"
#include "gpio-names.h"
#include "fuse.h"
#include "pm.h"
#include "common.h"
#include "tegra-board-id.h"

#ifdef CONFIG_BT_BLUESLEEP
static struct rfkill_gpio_platform_data roth_bt_rfkill_pdata = {
		.name           = "bt_rfkill",
		.shutdown_gpio  = TEGRA_GPIO_PQ7,
		.type           = RFKILL_TYPE_BLUETOOTH,
};

static struct platform_device roth_bt_rfkill_device = {
	.name = "rfkill_gpio",
	.id             = -1,
	.dev = {
		.platform_data = &roth_bt_rfkill_pdata,
	},
};

static struct resource roth_bluesleep_resources[] = {
	[0] = {
		.name = "gpio_host_wake",
			.start  = TEGRA_GPIO_PU6,
			.end    = TEGRA_GPIO_PU6,
			.flags  = IORESOURCE_IO,
	},
	[1] = {
		.name = "gpio_ext_wake",
			.start  = TEGRA_GPIO_PEE1,
			.end    = TEGRA_GPIO_PEE1,
			.flags  = IORESOURCE_IO,
	},
	[2] = {
		.name = "host_wake",
			.flags  = IORESOURCE_IRQ | IORESOURCE_IRQ_HIGHEDGE,
	},
};

static struct platform_device roth_bluesleep_device = {
	.name           = "bluesleep",
	.id             = -1,
	.num_resources  = ARRAY_SIZE(roth_bluesleep_resources),
	.resource       = roth_bluesleep_resources,
};

static noinline void __init roth_setup_bt_rfkill(void)
{
		roth_bt_rfkill_pdata.reset_gpio = TEGRA_GPIO_PQ6;
	platform_device_register(&roth_bt_rfkill_device);
}

static noinline void __init roth_setup_bluesleep(void)
{
	roth_bluesleep_resources[2].start =
		roth_bluesleep_resources[2].end =
			gpio_to_irq(TEGRA_GPIO_PU6);
	platform_device_register(&roth_bluesleep_device);
	return;
}
#elif defined CONFIG_BLUEDROID_PM
static struct resource roth_bluedroid_pm_resources[] = {
	[0] = {
		.name   = "shutdown_gpio",
		.start  = TEGRA_GPIO_PQ7,
		.end    = TEGRA_GPIO_PQ7,
		.flags  = IORESOURCE_IO,
	},
	[1] = {
		.name = "host_wake",
		.flags  = IORESOURCE_IRQ | IORESOURCE_IRQ_HIGHEDGE,
	},
	[2] = {
		.name = "gpio_ext_wake",
		.start  = TEGRA_GPIO_PEE1,
		.end    = TEGRA_GPIO_PEE1,
		.flags  = IORESOURCE_IO,
	},
	[3] = {
		.name = "gpio_host_wake",
		.start  = TEGRA_GPIO_PU6,
		.end    = TEGRA_GPIO_PU6,
		.flags  = IORESOURCE_IO,
	},
};

static struct platform_device roth_bluedroid_pm_device = {
	.name = "bluedroid_pm",
	.id             = 0,
	.num_resources  = ARRAY_SIZE(roth_bluedroid_pm_resources),
	.resource       = roth_bluedroid_pm_resources,
};

static noinline void __init roth_setup_bluedroid_pm(void)
{
	roth_bluedroid_pm_resources[1].start =
		roth_bluedroid_pm_resources[1].end =
				gpio_to_irq(TEGRA_GPIO_PU6);
	platform_device_register(&roth_bluedroid_pm_device);
}
#endif
static __initdata struct tegra_clk_init_table roth_clk_init_table[] = {
	/* name		parent		rate		enabled */
	{ "pll_m",	NULL,		0,		false},
	{ "hda",	"pll_p",	108000000,	false},
	{ "hda2codec_2x", "pll_p",	48000000,	false},
	{ "pwm",	"pll_p",	37000000,	false},
	{ "blink",	"clk_32k",	32768,		true},
	{ "i2s1",	"pll_a_out0",	0,		false},
	{ "i2s3",	"pll_a_out0",	0,		false},
	{ "i2s4",	"pll_a_out0",	0,		false},
	{ "spdif_out",	"pll_a_out0",	0,		false},
	{ "d_audio",	"clk_m",	12000000,	false},
	{ "dam0",	"clk_m",	12000000,	false},
	{ "dam1",	"clk_m",	12000000,	false},
	{ "dam2",	"clk_m",	12000000,	false},
	{ "audio1",	"i2s1_sync",	0,		false},
	{ "audio3",	"i2s3_sync",	0,		false},
	/* Setting vi_sensor-clk to true for validation purpose, will imapact
	 * power, later set to be false.*/
	{ "vi_sensor",	"pll_p",	150000000,	false},
	{ "cilab",	"pll_p",	150000000,	false},
	{ "cilcd",	"pll_p",	150000000,	false},
	{ "cile",	"pll_p",	150000000,	false},
	{ "i2c1",	"pll_p",	3200000,	false},
	{ "i2c2",	"pll_p",	3200000,	false},
	{ "i2c3",	"pll_p",	3200000,	false},
	{ "i2c4",	"pll_p",	3200000,	false},
	{ "i2c5",	"pll_p",	3200000,	false},
	{ NULL,		NULL,		0,		0},
};

static struct tegra_i2c_platform_data roth_i2c1_platform_data = {
	.adapter_nr	= 0,
	.bus_count	= 1,
	.bus_clk_rate	= { 100000, 0 },
	.scl_gpio		= {TEGRA_GPIO_I2C1_SCL, 0},
	.sda_gpio		= {TEGRA_GPIO_I2C1_SDA, 0},
	.arb_recovery = arb_lost_recovery,
};

static struct tegra_i2c_platform_data roth_i2c2_platform_data = {
	.adapter_nr	= 1,
	.bus_count	= 1,
	.bus_clk_rate	= { 100000, 0 },
	.is_clkon_always = true,
	.scl_gpio		= {TEGRA_GPIO_I2C2_SCL, 0},
	.sda_gpio		= {TEGRA_GPIO_I2C2_SDA, 0},
	.arb_recovery = arb_lost_recovery,
};

static struct tegra_i2c_platform_data roth_i2c3_platform_data = {
	.adapter_nr	= 2,
	.bus_count	= 1,
	.bus_clk_rate	= { 100000, 0 },
	.scl_gpio		= {TEGRA_GPIO_I2C3_SCL, 0},
	.sda_gpio		= {TEGRA_GPIO_I2C3_SDA, 0},
	.arb_recovery = arb_lost_recovery,
};

static struct tegra_i2c_platform_data roth_i2c4_platform_data = {
	.adapter_nr	= 3,
	.bus_count	= 1,
	.bus_clk_rate	= { 10000, 0 },
	.scl_gpio		= {TEGRA_GPIO_I2C4_SCL, 0},
	.sda_gpio		= {TEGRA_GPIO_I2C4_SDA, 0},
	.arb_recovery = arb_lost_recovery,
};

static struct tegra_i2c_platform_data roth_i2c5_platform_data = {
	.adapter_nr	= 4,
	.bus_count	= 1,
	.bus_clk_rate	= { 400000, 0 },
	.scl_gpio		= {TEGRA_GPIO_I2C5_SCL, 0},
	.sda_gpio		= {TEGRA_GPIO_I2C5_SDA, 0},
	.arb_recovery = arb_lost_recovery,
};

#if defined(CONFIG_ARCH_TEGRA_11x_SOC)
static struct i2c_board_info __initdata rt5640_board_info = {
	I2C_BOARD_INFO("rt5640", 0x1c),
};

static struct i2c_board_info __initdata roth_codec_tfa9887R_info = {
	I2C_BOARD_INFO("tfa9887R", 0x37),
};

static struct i2c_board_info __initdata roth_codec_tfa9887L_info = {
	I2C_BOARD_INFO("tfa9887L", 0x34),
};
#endif

static void roth_i2c_init(void)
{
	tegra11_i2c_device1.dev.platform_data = &roth_i2c1_platform_data;
	tegra11_i2c_device2.dev.platform_data = &roth_i2c2_platform_data;
	tegra11_i2c_device3.dev.platform_data = &roth_i2c3_platform_data;
	tegra11_i2c_device4.dev.platform_data = &roth_i2c4_platform_data;
	tegra11_i2c_device5.dev.platform_data = &roth_i2c5_platform_data;

	platform_device_register(&tegra11_i2c_device5);
	platform_device_register(&tegra11_i2c_device4);
	platform_device_register(&tegra11_i2c_device3);
	platform_device_register(&tegra11_i2c_device2);
	platform_device_register(&tegra11_i2c_device1);

	i2c_register_board_info(0, &rt5640_board_info, 1);
	i2c_register_board_info(0, &roth_codec_tfa9887R_info, 1);
	i2c_register_board_info(0, &roth_codec_tfa9887L_info, 1);
}

static struct platform_device *roth_uart_devices[] __initdata = {
	&tegra_uarta_device,
	&tegra_uartb_device,
	&tegra_uartc_device,
	&tegra_uartd_device,
};
static struct uart_clk_parent uart_parent_clk[] = {
	[0] = {.name = "clk_m"},
	[1] = {.name = "pll_p"},
#ifndef CONFIG_TEGRA_PLLM_RESTRICTED
	[2] = {.name = "pll_m"},
#endif
};

static struct tegra_uart_platform_data roth_uart_pdata;
static struct tegra_uart_platform_data roth_loopback_uart_pdata;

static void __init uart_debug_init(void)
{
	int debug_port_id;

	debug_port_id = uart_console_debug_init(3);
	if (debug_port_id < 0)
		return;

	roth_uart_devices[debug_port_id] = uart_console_debug_device;
}

static void __init roth_uart_init(void)
{
	struct clk *c;
	int i;

	for (i = 0; i < ARRAY_SIZE(uart_parent_clk); ++i) {
		c = tegra_get_clock_by_name(uart_parent_clk[i].name);
		if (IS_ERR_OR_NULL(c)) {
			pr_err("Not able to get the clock for %s\n",
						uart_parent_clk[i].name);
			continue;
		}
		uart_parent_clk[i].parent_clk = c;
		uart_parent_clk[i].fixed_clk_rate = clk_get_rate(c);
	}
	roth_uart_pdata.parent_clk_list = uart_parent_clk;
	roth_uart_pdata.parent_clk_count = ARRAY_SIZE(uart_parent_clk);
	roth_loopback_uart_pdata.parent_clk_list = uart_parent_clk;
	roth_loopback_uart_pdata.parent_clk_count =
						ARRAY_SIZE(uart_parent_clk);
	roth_loopback_uart_pdata.is_loopback = true;
	tegra_uarta_device.dev.platform_data = &roth_uart_pdata;
	tegra_uartb_device.dev.platform_data = &roth_uart_pdata;
	tegra_uartc_device.dev.platform_data = &roth_uart_pdata;
	tegra_uartd_device.dev.platform_data = &roth_uart_pdata;

	/* Register low speed only if it is selected */
	if (!is_tegra_debug_uartport_hs())
		uart_debug_init();

	platform_add_devices(roth_uart_devices,
				ARRAY_SIZE(roth_uart_devices));
}

static struct resource tegra_rtc_resources[] = {
	[0] = {
		.start = TEGRA_RTC_BASE,
		.end = TEGRA_RTC_BASE + TEGRA_RTC_SIZE - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = INT_RTC,
		.end = INT_RTC,
		.flags = IORESOURCE_IRQ,
	},
};

static struct platform_device tegra_rtc_device = {
	.name = "tegra_rtc",
	.id   = -1,
	.resource = tegra_rtc_resources,
	.num_resources = ARRAY_SIZE(tegra_rtc_resources),
};

static struct tegra_asoc_platform_data roth_audio_pdata = {
	.gpio_spkr_en		= TEGRA_GPIO_SPKR_EN,
	.gpio_hp_det		= TEGRA_GPIO_HP_DET,
	.gpio_hp_mute		= -1,
	.gpio_int_mic_en	= TEGRA_GPIO_INT_MIC_EN,
	.gpio_ext_mic_en	= TEGRA_GPIO_EXT_MIC_EN,
	.gpio_ldo1_en		= TEGRA_GPIO_LDO1_EN,
	.gpio_codec1 = TEGRA_GPIO_CODEC1_EN,
	.gpio_codec2 = TEGRA_GPIO_CODEC2_EN,
	.gpio_codec3 = TEGRA_GPIO_CODEC3_EN,
	.i2s_param[HIFI_CODEC]	= {
		.audio_port_id	= 1,
		.is_i2s_master	= 1,
		.i2s_mode	= TEGRA_DAIFMT_I2S,
	},
	.i2s_param[BT_SCO]	= {
		.audio_port_id	= 3,
		.is_i2s_master	= 1,
		.i2s_mode	= TEGRA_DAIFMT_DSP_A,
	},
};

static struct platform_device roth_audio_device = {
	.name	= "tegra-snd-rt5640",
	.id	= 0,
	.dev	= {
		.platform_data = &roth_audio_pdata,
	},
};

static struct platform_device tegra_camera = {
	.name = "tegra_camera",
	.id = -1,
};

#ifdef CONFIG_LEDS_PWM
static struct led_pwm roth_led_info[] = {
	{
		.name			= "roth-led",
		.default_trigger	= "none",
		.pwm_id			= 2,
		.active_low		= 0,
		.max_brightness		= 255,
		.pwm_period_ns		= 10000000,
	},
};

static struct led_pwm_platform_data roth_leds_pdata = {
	.leds		= roth_led_info,
	.num_leds	= ARRAY_SIZE(roth_led_info),
};

static struct platform_device roth_leds_pwm_device = {
	.name	= "leds_pwm",
	.id	= -1,
	.dev	= {
		.platform_data = &roth_leds_pdata,
	},
};

#else
static struct gpio_led roth_led_info[] = {
	{
		.name			= "roth-led",
		.default_trigger	= "none",
		.gpio			= TEGRA_GPIO_PQ3,
		.active_low		= 0,
		.retain_state_suspended	= 0,
		.default_state		= LEDS_GPIO_DEFSTATE_OFF,
	},
};

static struct gpio_led_platform_data roth_leds_pdata = {
	.leds		= roth_led_info,
	.num_leds	= ARRAY_SIZE(roth_led_info),
};

static struct platform_device roth_leds_gpio_device = {
	.name	= "leds-gpio",
	.id	= -1,
	.dev	= {
		.platform_data = &roth_leds_pdata,
	},
};
#endif

static struct platform_device *roth_devices[] __initdata = {
	&tegra_pmu_device,
	&tegra_rtc_device,
	&tegra_udc_device,
#if defined(CONFIG_TEGRA_IOVMM_SMMU) || defined(CONFIG_TEGRA_IOMMU_SMMU)
	&tegra_smmu_device,
#endif
#if defined(CONFIG_TEGRA_AVP)
	&tegra_avp_device,
#endif
	&tegra_camera,
#if defined(CONFIG_CRYPTO_DEV_TEGRA_SE)
	&tegra11_se_device,
#endif
	&tegra_ahub_device,
	&tegra_dam_device0,
	&tegra_dam_device1,
	&tegra_dam_device2,
	&tegra_i2s_device1,
	&tegra_i2s_device3,
	&tegra_i2s_device4,
	&tegra_spdif_device,
	&spdif_dit_device,
	&bluetooth_dit_device,
	&tegra_pcm_device,
	&roth_audio_device,
	&tegra_hda_device,
#if defined(CONFIG_CRYPTO_DEV_TEGRA_AES)
	&tegra_aes_device,
#endif

#if CONFIG_LEDS_PWM
	&tegra_pwfm2_device,
	&roth_leds_pwm_device,
#else
	&roth_leds_gpio_device,
#endif
};

#ifdef CONFIG_USB_SUPPORT
static struct tegra_usb_platform_data tegra_udc_pdata = {
	.port_otg = true,
	.has_hostpc = true,
	.phy_intf = TEGRA_USB_PHY_INTF_UTMI,
	.op_mode = TEGRA_USB_OPMODE_DEVICE,
	.u_data.dev = {
		.vbus_pmu_irq = 0,
		.vbus_gpio = -1,
		.charging_supported = false,
		.remote_wakeup_supported = false,
	},
	.u_cfg.utmi = {
		.hssync_start_delay = 0,
		.elastic_limit = 16,
		.idle_wait_delay = 17,
		.term_range_adj = 6,
		.xcvr_setup = 8,
		.xcvr_lsfslew = 2,
		.xcvr_lsrslew = 2,
		.xcvr_setup_offset = 0,
		.xcvr_use_fuses = 1,
	},
};

static struct tegra_usb_platform_data tegra_ehci1_utmi_pdata = {
	.port_otg = true,
	.has_hostpc = true,
	.unaligned_dma_buf_supported = false,
	.phy_intf = TEGRA_USB_PHY_INTF_UTMI,
	.op_mode = TEGRA_USB_OPMODE_HOST,
	.u_data.host = {
		.vbus_gpio = -1,
		.hot_plug = true,
		.remote_wakeup_supported = true,
		.power_off_on_suspend = false,
	},
	.u_cfg.utmi = {
		.hssync_start_delay = 0,
		.elastic_limit = 16,
		.idle_wait_delay = 17,
		.term_range_adj = 6,
		.xcvr_setup = 15,
		.xcvr_lsfslew = 2,
		.xcvr_lsrslew = 2,
		.xcvr_setup_offset = 0,
		.xcvr_use_fuses = 1,
		.vbus_oc_map = 0x4,
	},
};

static struct tegra_usb_platform_data tegra_ehci3_utmi_pdata = {
	.port_otg = false,
	.has_hostpc = true,
	.unaligned_dma_buf_supported = false,
	.phy_intf = TEGRA_USB_PHY_INTF_UTMI,
	.op_mode = TEGRA_USB_OPMODE_HOST,
	.u_data.host = {
		.vbus_gpio = -1,
		.hot_plug = true,
		.remote_wakeup_supported = true,
		.power_off_on_suspend = false,
	},
	.u_cfg.utmi = {
	.hssync_start_delay = 0,
		.elastic_limit = 16,
		.idle_wait_delay = 17,
		.term_range_adj = 6,
		.xcvr_setup = 8,
		.xcvr_lsfslew = 2,
		.xcvr_lsrslew = 2,
		.xcvr_setup_offset = 0,
		.xcvr_use_fuses = 1,
		.vbus_oc_map = 0x5,
	},
};

static struct tegra_usb_otg_data tegra_otg_pdata = {
	.ehci_device = &tegra_ehci1_device,
	.ehci_pdata = &tegra_ehci1_utmi_pdata,
};

static void roth_usb_init(void)
{
	tegra_otg_device.dev.platform_data = &tegra_otg_pdata;
	platform_device_register(&tegra_otg_device);

	/* Setup the udc platform data */
	tegra_udc_device.dev.platform_data = &tegra_udc_pdata;

	tegra_ehci3_device.dev.platform_data = &tegra_ehci3_utmi_pdata;
	platform_device_register(&tegra_ehci3_device);
}

#else
static void roth_usb_init(void) { }
#endif

static void roth_audio_init(void)
{
	struct board_info board_info;

	tegra_get_board_info(&board_info);

	roth_audio_pdata.codec_name = "rt5640.0-001c";
	roth_audio_pdata.codec_dai_name = "rt5640-aif1";
}


static void __init tegra_roth_init(void)
{
	tegra_clk_init_from_table(roth_clk_init_table);
	tegra_clk_vefify_parents();
	tegra_soc_device_init("roth");
	tegra_enable_pinmux();
	roth_pinmux_init();
	roth_i2c_init();
	roth_usb_init();
	roth_uart_init();
	roth_audio_init();
	platform_add_devices(roth_devices, ARRAY_SIZE(roth_devices));
	tegra_ram_console_debug_init();
	tegra_io_dpd_init();
	roth_regulator_init();
	roth_sdhci_init();
	roth_suspend_init();
	roth_emc_init();
	roth_edp_init();
	roth_panel_init();
	roth_kbc_init();
	roth_pmon_init();
#ifdef CONFIG_BT_BLUESLEEP
	roth_setup_bluesleep();
	roth_setup_bt_rfkill();
#elif defined CONFIG_BLUEDROID_PM
	roth_setup_bluedroid_pm();
#endif
	tegra_release_bootloader_fb();
#ifdef CONFIG_TEGRA_WDT_RECOVERY
	tegra_wdt_recovery_init();
#endif
	tegra_serial_debug_init(TEGRA_UARTD_BASE, INT_WDT_CPU, NULL, -1, -1);
	roth_sensors_init();
	roth_soctherm_init();
	roth_fan_init();
}

static void __init roth_ramconsole_reserve(unsigned long size)
{
	tegra_ram_console_debug_reserve(SZ_1M);
}

static void __init tegra_roth_dt_init(void)
{
	tegra_roth_init();

#ifdef CONFIG_USE_OF
	of_platform_populate(NULL,
		of_default_bus_match_table, NULL, NULL);
#endif
}

static void __init tegra_roth_reserve(void)
{
#if defined(CONFIG_NVMAP_CONVERT_CARVEOUT_TO_IOVMM)
	/* 1920*1200*4*2 = 18432000 bytes */
	tegra_reserve(0, SZ_16M + SZ_2M, SZ_4M);
#else
	tegra_reserve(SZ_128M, SZ_16M + SZ_2M, SZ_4M);
#endif
	roth_ramconsole_reserve(SZ_1M);
}

static const char * const roth_dt_board_compat[] = {
	"nvidia,roth",
	NULL
};

MACHINE_START(ROTH, "roth")
	.atag_offset	= 0x100,
	.soc		= &tegra_soc_desc,
	.map_io		= tegra_map_common_io,
	.reserve	= tegra_roth_reserve,
	.init_early	= tegra11x_init_early,
	.init_irq	= tegra_init_irq,
	.handle_irq	= gic_handle_irq,
	.timer		= &tegra_timer,
	.init_machine	= tegra_roth_dt_init,
	.restart	= tegra_assert_system_reset,
	.dt_compat	= roth_dt_board_compat,
MACHINE_END
