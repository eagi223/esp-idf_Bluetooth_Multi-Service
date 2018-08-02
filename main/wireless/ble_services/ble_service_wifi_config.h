
#ifndef WIFI_CONFIG_SERVICE_H_
    #define WIFI_CONFIG_SERVICE_H_
    #include <stdint.h>

    #include "esp_gatt_defs.h"
    #include "esp_gatts_api.h"

	enum
	{
		// Service index
		WIFI_SERV,

        WIFI_CONFIG_CHAR,
        WIFI_CONFIG_VAL,
        WIFI_CONFIG_DESCR,
        WIFI_CONFIG_CFG,

        WIFI_INFO_CHAR,
        WIFI_INFO_VAL,
        WIFI_INFO_DESCR,
        WIFI_INFO_CFG,

		WIFI_NB,
	};

    uint16_t wifi_handle_table[WIFI_NB];

    extern const uint16_t WIFI_SERV_uuid;
    extern const esp_gatts_attr_db_t wifi_serv_gatt_db[WIFI_NB];

    uint16_t getAttributeIndexByWifiHandle(uint16_t attributeHandle);
    void handleWifiReadEvent(int attrIndex, esp_ble_gatts_cb_param_t* param, esp_gatt_rsp_t* rsp);
    void handleWifiWriteEvent(int attrIndex, esp_ble_gatts_cb_param_t* param, esp_gatt_rsp_t* rsp);

#endif /* WIFI_CONFIG_SERVICE_H_ */
