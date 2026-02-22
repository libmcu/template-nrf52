/*
 * SPDX-FileCopyrightText: 2026 권경환 Kyunghwan Kwon <k@libmcu.org>
 *
 * SPDX-License-Identifier: MIT
 */

#include "libmcu/board.h"
#include "libmcu/compiler.h"

#include <zephyr/kernel.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/drivers/hwinfo.h>
#include <zephyr/random/random.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/drivers/usb/usb_dc.h>
#include <zephyr/logging/log.h>

#include <stdio.h>
#include <string.h>

LOG_MODULE_REGISTER(usb, LOG_LEVEL_INF);

static void on_usb_status(enum usb_dc_status_code status,
			  const uint8_t *param)
{
	ARG_UNUSED(param);

	switch (status) {
	case USB_DC_CONNECTED:
		LOG_INF("USB connected");
		break;
	case USB_DC_CONFIGURED:
		LOG_INF("USB CDC ACM ready");
		break;
	case USB_DC_DISCONNECTED:
		LOG_INF("USB disconnected");
		break;
	case USB_DC_SUSPEND:
		LOG_INF("USB suspended");
		break;
	case USB_DC_RESUME:
		LOG_INF("USB resumed");
		break;
	default:
		break;
	}
}

void board_init(void)
{
	int ret = usb_enable(on_usb_status);

	if (ret != 0) {
		LOG_ERR("Failed to enable USB (err %d)", ret);
		return;
	}
}

void board_reboot(void)
{
	sys_reboot(SYS_REBOOT_COLD);
}

board_reboot_reason_t board_get_reboot_reason(void)
{
	uint32_t cause = 0;

	hwinfo_get_reset_cause(&cause);
	hwinfo_clear_reset_cause();

	if (cause & RESET_WATCHDOG) {
		return BOARD_REBOOT_WDT;
	} else if (cause & RESET_PIN) {
		return BOARD_REBOOT_PIN;
	} else if (cause & RESET_SOFTWARE) {
		return BOARD_REBOOT_SOFT;
	} else if (cause & RESET_POR) {
		return BOARD_REBOOT_POWER;
	} else if (cause & RESET_DEBUG) {
		return BOARD_REBOOT_DEBUGGER;
	} else if (cause & RESET_BROWNOUT) {
		return BOARD_REBOOT_BROWNOUT;
	}

	return BOARD_REBOOT_UNKNOWN;
}

const char *board_get_serial_number_string(void)
{
	static char sn[17];

	if (sn[0] == '\0') {
		uint8_t dev_id[8];
		ssize_t len = hwinfo_get_device_id(dev_id, sizeof(dev_id));

		if (len > 0) {
			snprintf(sn, sizeof(sn),
				"%02x%02x%02x%02x%02x%02x%02x%02x",
				dev_id[0], dev_id[1], dev_id[2], dev_id[3],
				dev_id[4], dev_id[5], dev_id[6], dev_id[7]);
		} else {
			strncpy(sn, "unknown", sizeof(sn) - 1);
		}
	}

	return sn;
}

const char *board_name(void)
{
	return "madi_nrf52840";
}

LIBMCU_NO_INSTRUMENT
uint32_t board_get_time_since_boot_ms(void)
{
	return k_uptime_get_32();
}

LIBMCU_NO_INSTRUMENT
uint64_t board_get_time_since_boot_us(void)
{
	return (uint64_t)k_uptime_get() * 1000U;
}

LIBMCU_NO_INSTRUMENT
void *board_get_current_thread(void)
{
	return k_current_get();
}

uint32_t board_random(void)
{
	return sys_rand32_get();
}
