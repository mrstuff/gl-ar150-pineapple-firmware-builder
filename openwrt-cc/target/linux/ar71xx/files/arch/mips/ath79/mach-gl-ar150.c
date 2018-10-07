/*
 *  GL_ar150 board support
 *
 *  Copyright (C) 2011 dongyuqi <729650915@qq.com>
 *  Copyright (C) 2011-2012 Gabor Juhos <juhosg@openwrt.org>
 *  Copyright (C) 2013 alzhao <alzhao@gmail.com>
 *  Copyright (C) 2014 Michel Stempin <michel.stempin@wanadoo.fr>
 *
 *  This program is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License version 2 as published
 *  by the Free Software Foundation.
*/

#include <linux/gpio.h>

#include <asm/mach-ath79/ath79.h>
#include <asm/mach-ath79/ar71xx_regs.h>

#include "common.h"
#include "dev-eth.h"
#include "dev-gpio-buttons.h"
#include "dev-leds-gpio.h"
#include "dev-m25p80.h"
#include "dev-usb.h"
#include "dev-wmac.h"
#include "machtypes.h"

#define GL_AR150_GPIO_LED_WLAN		   0
#define GL_AR150_GPIO_LED_LAN		   13
#define GL_AR150_GPIO_LED_WAN		   15

#define GL_AR150_GPIO_USB_POW         6
#define GL_AR150_GPIO_BTN_MANUAL      7
#define GL_AR150_GPIO_BTN_AUTO	   	   8
#define GL_AR150_GPIO_BTN_RESET	   11

#define GL_AR150_KEYS_POLL_INTERVAL   20	/* msecs */
#define GL_AR150_KEYS_DEBOUNCE_INTERVAL	(3 * GL_AR150_KEYS_POLL_INTERVAL)

#define GL_AR150_MAC0_OFFSET	0x0000
#define GL_AR150_MAC1_OFFSET	0x0006
#define GL_AR150_CALDATA_OFFSET	0x1000
#define GL_AR150_WMAC_MAC_OFFSET	0x0000

static struct gpio_led gl_ar150_leds_gpio[] __initdata = {
	{
		.name = "gl_ar150:wlan",
		.gpio = GL_AR150_GPIO_LED_WLAN,
		.active_low = 0,
	},
	{
		.name = "gl_ar150:lan",
		.gpio = GL_AR150_GPIO_LED_LAN,
		.active_low = 0,
	},
	{
		.name = "gl_ar150:wan",
		.gpio = GL_AR150_GPIO_LED_WAN,
		.active_low = 0,
 		.default_state = 1,
	},
	{
		.name = "gl_ar150:usbpow",
		.gpio = GL_AR150_GPIO_USB_POW,
		.active_low = 0,
		.default_state = 1,
	},
};

static struct gpio_keys_button gl_ar150_gpio_keys[] __initdata = {
	{
		.desc = "BTN_7",
		.type = EV_KEY,
		.code = BTN_7,
		.debounce_interval = GL_AR150_KEYS_DEBOUNCE_INTERVAL,
		.gpio = GL_AR150_GPIO_BTN_MANUAL,
		.active_low = 1,
	},
	{
		.desc = "BTN_8",
		.type = EV_KEY,
		.code = BTN_8,
		.debounce_interval = GL_AR150_KEYS_DEBOUNCE_INTERVAL,
		.gpio = GL_AR150_GPIO_BTN_AUTO,
		.active_low = 1,
	},
	{
		.desc = "reset",
		.type = EV_KEY,
		.code = KEY_RESTART,
		.debounce_interval = GL_AR150_KEYS_DEBOUNCE_INTERVAL,
		.gpio = GL_AR150_GPIO_BTN_RESET,
		.active_low = 1,
	},
};

static void __init gl_ar150_gpio_setup(void)
{
	u32 t;

	ath79_gpio_function_disable(AR933X_GPIO_FUNC_ETH_SWITCH_LED0_EN |
				     AR933X_GPIO_FUNC_ETH_SWITCH_LED1_EN |
				     AR933X_GPIO_FUNC_ETH_SWITCH_LED2_EN |
				     AR933X_GPIO_FUNC_ETH_SWITCH_LED3_EN |
				     AR933X_GPIO_FUNC_ETH_SWITCH_LED4_EN);

	t = ath79_reset_rr(AR933X_RESET_REG_BOOTSTRAP);
	t |= AR933X_BOOTSTRAP_MDIO_GPIO_EN;
	ath79_reset_wr(AR933X_RESET_REG_BOOTSTRAP, t);

	gpio_request_one(GL_AR150_GPIO_USB_POW,
			 GPIOF_OUT_INIT_LOW | GPIOF_EXPORT_DIR_FIXED,
			 "USB power");
}

static void __init gl_ar150_setup(void)
{

	/* ART base address */
	u8 *art = (u8 *) KSEG1ADDR(0x1fff0000);

	gl_ar150_gpio_setup();

	/* disable PHY_SWAP and PHY_ADDR_SWAP bits */
	ath79_setup_ar933x_phy4_switch(false, false);

	/* register flash. */
	ath79_register_m25p80(NULL);

	/* register gpio LEDs and keys */
	ath79_register_leds_gpio(-1, ARRAY_SIZE(gl_ar150_leds_gpio),
				 gl_ar150_leds_gpio);
	ath79_register_gpio_keys_polled(-1, GL_AR150_KEYS_POLL_INTERVAL,
					ARRAY_SIZE(gl_ar150_gpio_keys),
					gl_ar150_gpio_keys);

	/* enable usb */
	ath79_register_usb();

	/* register eth0 as WAN, eth1 as LAN */
	ath79_init_mac(ath79_eth0_data.mac_addr, art+GL_AR150_MAC0_OFFSET, 0);
	ath79_init_mac(ath79_eth1_data.mac_addr, art+GL_AR150_MAC1_OFFSET, 0);
	ath79_register_mdio(0, 0x0);
	ath79_register_eth(0);
	ath79_register_eth(1);

	/* register wireless mac with cal data */
	ath79_register_wmac(art + GL_AR150_CALDATA_OFFSET, NULL);
}

MIPS_MACHINE(ATH79_MACH_GL_AR150, "GL-AR150", "WiFi Pineapple NANO",gl_ar150_setup);
