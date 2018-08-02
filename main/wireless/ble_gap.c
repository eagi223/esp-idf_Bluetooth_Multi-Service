#include "ble_gap.h"

#include "esp_log.h"

#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_gatt_defs.h"
#include "esp_bt_defs.h"
#include "esp_bt_device.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"

#define TAG "ble gap"

#define BLE_DEVICE_NAME "BLE Test Device"

static uint8_t adv_config_done = 0;

#ifdef CONFIG_SET_RAW_ADV_DATA
	static uint8_t raw_adv_data[] = {
			/* flags */
			0x02, 0x01, 0x06,

			/* tx power*/
			0x02, 0x0a, 0xeb,

			/* service uuid */
			0x03, 0x03, 0xFF, 0x00,

			/* device name */
			0x0f, 0x09, 'E', 'S', 'P', '_', 'G', 'A', 'T', 'T', 'S', '_', 'D','E', 'M', 'O'
	};

	static uint8_t raw_scan_rsp_data[] = {
			/* flags */
			0x02, 0x01, 0x06,

			/* tx power */
			0x02, 0x0a, 0xeb,

			/* service uuid */
			0x03, 0x03, 0xFF,0x00

	};
#else
	static uint8_t service_uuid[16] = {
		/* LSB <--------------------------------------------------------------------------------> MSB */
		//first uuid, 16bit, [12],[13] is the value
		0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00,
	};

	/* The length of adv data must be less than 31 bytes */
	static esp_ble_adv_data_t adv_data = {
		.set_scan_rsp        = false,
		.include_name        = true,
		.include_txpower     = true,
		.min_interval        = 0x20,
		.max_interval        = 0x40,
		.appearance          = 0x00,
		.manufacturer_len    = 0,    //TEST_MANUFACTURER_DATA_LEN,
		.p_manufacturer_data = NULL, //test_manufacturer,
		.service_data_len    = 0,
		.p_service_data      = NULL,
		.service_uuid_len    = sizeof(service_uuid),
		.p_service_uuid      = service_uuid,
		.flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
	};

	// scan response data
	static esp_ble_adv_data_t scan_rsp_data = {
		.set_scan_rsp        = true,
		.include_name        = true,
		.include_txpower     = true,
		.min_interval        = 0x20,
		.max_interval        = 0x40,
		.appearance          = 0x00,
		.manufacturer_len    = 0, //TEST_MANUFACTURER_DATA_LEN,
		.p_manufacturer_data = NULL, //&test_manufacturer[0],
		.service_data_len    = 0,
		.p_service_data      = NULL,
		.service_uuid_len    = sizeof(service_uuid),
		.p_service_uuid      = service_uuid,
		.flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
	};
#endif /* CONFIG_SET_RAW_ADV_DATA */

static esp_ble_adv_params_t adv_params = {
    .adv_int_min         = 0x20,
    .adv_int_max         = 0x40,
    .adv_type            = ADV_TYPE_IND,
    .own_addr_type       = BLE_ADDR_TYPE_PUBLIC,
    .channel_map         = ADV_CHNL_ALL,
    .adv_filter_policy   = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};



static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
	ESP_LOGI(TAG, "gap event handler: %d", event);
    switch (event) {
    #ifdef CONFIG_SET_RAW_ADV_DATA
        case ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT:
            adv_config_done &= (~ADV_CONFIG_FLAG);
            if (adv_config_done == 0)
            {
                esp_ble_gap_start_advertising(&adv_params);
            }
            break;
        case ESP_GAP_BLE_SCAN_RSP_DATA_RAW_SET_COMPLETE_EVT:
            adv_config_done &= (~SCAN_RSP_CONFIG_FLAG);
            if (adv_config_done == 0)
            {
                esp_ble_gap_start_advertising(&adv_params);
            }
            break;
    #else
        case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
            adv_config_done &= (~ADV_CONFIG_FLAG);
            if (adv_config_done == 0)
            {
            	ESP_LOGI(TAG,"ADV Config Done\n");
                esp_ble_gap_start_advertising(&adv_params);
            }
            break;
        case ESP_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT:
            adv_config_done &= (~SCAN_RSP_CONFIG_FLAG);
            if (adv_config_done == 0){
                esp_ble_gap_start_advertising(&adv_params);
            }
            break;
    #endif
	case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
		/* advertising start complete event to indicate advertising start successfully or failed */
		if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS)
		{
			ESP_LOGE(TAG,"advertising start failed");
		}
		else
		{
			ESP_LOGI(TAG,"advertising start successfully");
		}
		break;
	case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
		if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS)
		{
			ESP_LOGE(TAG,"Advertising stop failed");
		}
		else
		{
			ESP_LOGI(TAG,"Stop adv successfully\n");
		}
		break;
	case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
		ESP_LOGI(TAG,"update connection params status = %d, min_int = %d, max_int = %d,conn_int = %d,latency = %d, timeout = %d",
			  param->update_conn_params.status,
			  param->update_conn_params.min_int,
			  param->update_conn_params.max_int,
			  param->update_conn_params.conn_int,
			  param->update_conn_params.latency,
			  param->update_conn_params.timeout);
		break;
	default:
		break;
    }
}



void ble_gap_setAdvertisingData(void)
{
    esp_err_t set_dev_name_ret = esp_ble_gap_set_device_name(BLE_DEVICE_NAME);
    if (set_dev_name_ret)
    {
        ESP_LOGE(TAG,"set device name failed, error code = %x", set_dev_name_ret);
    }

	#ifdef CONFIG_SET_RAW_ADV_DATA

				esp_err_t raw_adv_ret = esp_ble_gap_config_adv_data_raw(raw_adv_data, sizeof(raw_adv_data));
				if (raw_adv_ret)
				{
					LOGE(TAG,"config raw adv data failed, error code = %x ", raw_adv_ret);
				}

				adv_config_done |= ADV_CONFIG_FLAG;

				esp_err_t raw_scan_ret = esp_ble_gap_config_scan_rsp_data_raw(raw_scan_rsp_data, sizeof(raw_scan_rsp_data));
				if (raw_scan_ret)
				{
					LOGE(TAG,"config raw scan rsp data failed, error code = %x", raw_scan_ret);
				}

				adv_config_done |= SCAN_RSP_CONFIG_FLAG;

	#else

				//config adv data
				esp_err_t ret = esp_ble_gap_config_adv_data(&adv_data);
				if (ret)
				{
					ESP_LOGE(TAG,"config adv data failed, error code = %x", ret);
				}

				adv_config_done |= ADV_CONFIG_FLAG;

				//config scan response data
				ret = esp_ble_gap_config_adv_data(&scan_rsp_data);
				if (ret)
				{
					ESP_LOGE(TAG,"config scan response data failed, error code = %x", ret);
				}

				adv_config_done |= SCAN_RSP_CONFIG_FLAG;

	#endif
}



void ble_gap_startAdvertising(void)
{
	esp_ble_gap_start_advertising(&adv_params);
}



bool ble_gap_init(void)
{
	esp_err_t err = esp_ble_gap_register_callback(gap_event_handler);
	if( err )
	{
		ESP_LOGE(TAG,"Gap register failed with err: %x", err);
		return false;
	}

	return true;
}



void ble_gap_deinit(void)
{
	// Do nothing
}


