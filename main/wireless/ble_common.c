
#include "ble_common.h"

const uint16_t primary_service_uuid         = ESP_GATT_UUID_PRI_SERVICE;
const uint16_t character_client_config_uuid = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;
const uint16_t character_declaration_uuid   = ESP_GATT_UUID_CHAR_DECLARE;
const uint16_t character_description        = ESP_GATT_UUID_CHAR_DESCRIPTION; //0x2901

const uint8_t char_prop_notify            = ESP_GATT_CHAR_PROP_BIT_NOTIFY;
const uint8_t char_prop_read              = ESP_GATT_CHAR_PROP_BIT_READ;
const uint8_t char_prop_read_notify       = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_NOTIFY;
const uint8_t char_prop_read_write        = ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_READ;
const uint8_t char_prop_read_write_notify = ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_NOTIFY;
const uint8_t char_prop_write             = ESP_GATT_CHAR_PROP_BIT_WRITE;
const uint8_t char_prop_write_notify      = ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_NOTIFY;
