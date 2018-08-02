
#include "ble_interface.h"
#include "ble_gap.h"
#include "ble_gatt_server.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"

#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TAG "ble interface"

esp_err_t ble_interface_init(void)
{
	esp_err_t err;

	ESP_LOGI(TAG,"Initializing Bluetooth Driver");

	esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();

	err = esp_bt_controller_init(&bt_cfg);

	if (err)
	{
		ESP_LOGE(TAG,"initialize controller failed");
		return err;
	}

	err = esp_bt_controller_enable(ESP_BT_MODE_BLE);
	if (err)
	{
		ESP_LOGE(TAG,"enable controller failed");
		return err;
	}
	err = esp_bluedroid_init();
	if (err)
	{
		ESP_LOGE(TAG,"init bluetooth failed");
		return err;
	}
	err = esp_bluedroid_enable();
	if (err)
	{
		ESP_LOGE(TAG,"enable bluetooth failed");
		return err;
	}

	ble_gap_init();
	ble_gatt_server_init();

	ESP_LOGI(TAG,"Bluetooth Driver Initialized");

	return err;
}

esp_err_t ble_interface_deinit(void)
{
	esp_err_t err;

	ble_gatt_server_deinit();
	ble_gap_deinit();

	err = esp_bluedroid_disable();
	if (err)
	{
		ESP_LOGE(TAG,"disable bluedroid failed");
		return err;
	}

	err = esp_bluedroid_deinit();
	if (err)
	{
		ESP_LOGE(TAG,"deinitialize bluedroid failed");
		return err;
	}

	err = esp_bt_controller_disable();
	if (err)
	{
		ESP_LOGE(TAG,"disable bt controller failed");
		return err;
	}

	err = esp_bt_controller_deinit();
	if (err)
	{
		ESP_LOGE(TAG,"deinitialize bt controller failed");
		return err;
	}

	return err;
}