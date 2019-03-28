
#include "ble_service_wifi_config.h"

#include "esp_gatts_api.h"
#include "esp_log.h"

#include "../ble_common.h"

#include <string.h>

#define TAG "ble wifi service"

//========================================================================
//		Wifi Service
//========================================================================

const uint16_t WIFI_SERV_uuid 				        = 0x00FF;
static const uint16_t WIFI_SERV_CHAR_info_uuid      = 0xFF01;
static const uint16_t WIFI_SERV_CHAR_config_uuid    = 0xFF02;     

static const uint8_t WIFI_SERV_descr[]              = "Wifi Service";
static const uint8_t WIFI_SERV_CHAR_info_descr[]    = "Wifi Info";
static const uint8_t WIFI_SERV_CHAR_config_descr[]  = "Wifi Config";

static uint8_t WIFI_SERV_CHAR_info_ccc[2]           = {0x00,0x00};
static uint8_t WIFI_SERV_CHAR_config_ccc[2]         = {0x00,0x00};

static uint8_t WIFI_SERV_CHAR_info_val[20]          = {0x00};
static uint8_t WIFI_SERV_CHAR_config_val[20]        = {0x00};


const esp_gatts_attr_db_t wifi_serv_gatt_db[WIFI_NB] =
{
	[WIFI_SERV] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *) &primary_service_uuid, ESP_GATT_PERM_READ, sizeof(uint16_t), sizeof(WIFI_SERV_uuid), (uint8_t *)&WIFI_SERV_uuid}},

	[WIFI_CONFIG_CHAR] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *) &character_declaration_uuid, ESP_GATT_PERM_READ, CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_write_notify}},
    [WIFI_CONFIG_VAL] = {{ESP_GATT_RSP_BY_APP}, {ESP_UUID_LEN_16, (uint8_t *) &WIFI_SERV_CHAR_config_uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, sizeof(uint16_t), 0, NULL}},
    [WIFI_CONFIG_DESCR] = {{ESP_GATT_RSP_BY_APP}, {ESP_UUID_LEN_16, (uint8_t *) &character_description, ESP_GATT_PERM_READ, CHAR_DECLARATION_SIZE, 0, NULL}},
	[WIFI_CONFIG_CFG] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *) &character_client_config_uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, sizeof(uint16_t), sizeof(WIFI_SERV_CHAR_info_ccc), (uint8_t *)WIFI_SERV_CHAR_info_ccc}},

	[WIFI_INFO_CHAR] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *) &character_declaration_uuid, ESP_GATT_PERM_READ, CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_write_notify}},
    [WIFI_INFO_VAL] = {{ESP_GATT_RSP_BY_APP}, {ESP_UUID_LEN_16, (uint8_t *) &WIFI_SERV_CHAR_info_uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, sizeof(uint16_t), 0, NULL}},
    [WIFI_INFO_DESCR] = {{ESP_GATT_RSP_BY_APP}, {ESP_UUID_LEN_16, (uint8_t *) &character_description, ESP_GATT_PERM_READ, CHAR_DECLARATION_SIZE, 0, NULL}},
    [WIFI_INFO_CFG] = {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *) &character_client_config_uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, sizeof(uint16_t), sizeof(WIFI_SERV_CHAR_config_ccc), (uint8_t *)WIFI_SERV_CHAR_config_ccc}},
};


uint16_t getAttributeIndexByWifiHandle(uint16_t attributeHandle)
{
	// Get the attribute index in the attribute table by the returned handle

    uint16_t attrIndex = WIFI_NB;
    uint16_t index;

    for(index = 0; index < WIFI_NB; ++index)
    {
        if( wifi_handle_table[index] == attributeHandle )
        {
            attrIndex = index;
            break;
        }
    }

    return attrIndex;
}


void handleWifiReadEvent(int attrIndex, esp_ble_gatts_cb_param_t* param, esp_gatt_rsp_t* rsp)
{
    switch( attrIndex )
    {
    // Characteristic read values
    case WIFI_INFO_VAL:
	    memset(rsp->attr_value.value, 0, sizeof(rsp->attr_value.value));
	    memcpy(rsp->attr_value.value, WIFI_SERV_CHAR_info_val, sizeof(WIFI_SERV_CHAR_info_val));
	    rsp->attr_value.len = sizeof(WIFI_SERV_CHAR_info_val);
	    break;

    case WIFI_CONFIG_VAL:
	    memset(rsp->attr_value.value, 0, sizeof(rsp->attr_value.value));
	    memcpy(rsp->attr_value.value, WIFI_SERV_CHAR_config_val, sizeof(WIFI_SERV_CHAR_config_val));
	    rsp->attr_value.len = sizeof(WIFI_SERV_CHAR_config_val);
	    break;

    // Characteristic descriptions
    case WIFI_INFO_DESCR:
    	memset(rsp->attr_value.value, 0, sizeof(rsp->attr_value.value));
        memcpy(rsp->attr_value.value, WIFI_SERV_CHAR_info_descr, sizeof(WIFI_SERV_CHAR_info_descr));
        rsp->attr_value.len = sizeof(WIFI_SERV_CHAR_info_descr);
        break;

    case WIFI_CONFIG_DESCR:
	    memset(rsp->attr_value.value, 0, sizeof(rsp->attr_value.value));
        memcpy(rsp->attr_value.value, WIFI_SERV_CHAR_config_descr, sizeof(WIFI_SERV_CHAR_config_descr));
        rsp->attr_value.len = sizeof(WIFI_SERV_CHAR_config_descr);
        break;
    }
}

void handleWifiWriteEvent(int attrIndex, esp_ble_gatts_cb_param_t* param, esp_gatt_rsp_t* rsp)
{
    switch( attrIndex )
    {
    case WIFI_INFO_VAL:
        /*
         *  Handle any writes to Wifi Info char here
         */

        // This prints the first byte written to the characteristic
        ESP_LOGI(TAG, "Wifi Info characteristic written with %02x", param->write.value[0]);
	    break;

    case WIFI_CONFIG_VAL:
        /*
         *  Handle any writes to Wifi Val char here
         */
	    break;
    }
}
