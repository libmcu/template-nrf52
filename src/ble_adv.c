/*
 * SPDX-FileCopyrightText: 2026 권경환 Kyunghwan Kwon <k@libmcu.org>
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(ble, LOG_LEVEL_INF);

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_NAME_COMPLETE,
		CONFIG_BT_DEVICE_NAME,
		sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

int ble_adv_start(void)
{
	int err;

	err = bt_enable(NULL);
	if (err) {
		LOG_ERR("bt_enable failed (err %d)", err);
		return err;
	}

	LOG_INF("Bluetooth initialized");

	err = bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		LOG_ERR("bt_le_adv_start failed (err %d)", err);
		return err;
	}

	LOG_INF("Advertising started: \"%s\"", CONFIG_BT_DEVICE_NAME);

	return 0;
}
