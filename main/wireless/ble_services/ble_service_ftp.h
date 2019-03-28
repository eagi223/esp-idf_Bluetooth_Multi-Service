
#ifndef BLE_SERVICE_FTP_H_
    #define BLE_SERVICE_FTP_H_

    #include <stdint.h>

    #include "esp_gatt_defs.h"
    #include "esp_gatts_api.h"

	enum
	{
		// Service index
		FTP_SERV,

        FTP_DATA_CHAR,
		FTP_DATA_VAL,
		FTP_DATA_DESCR,
		FTP_DATA_CFG,

		FTP_SERV_NUM_ATTR,
	};

    uint16_t ftp_handle_table[FTP_SERV_NUM_ATTR];

    extern const uint16_t FTP_SERV_uuid;
    extern const esp_gatts_attr_db_t ftp_serv_gatt_db[FTP_SERV_NUM_ATTR];

    uint16_t getAttributeIndexByFtpHandle(uint16_t attributeHandle);
    void handleFtpReadEvent(int attrIndex, esp_ble_gatts_cb_param_t* param, esp_gatt_rsp_t* rsp);
    void handleFtpWriteEvent(int attrIndex, const uint8_t *char_val_p, uint16_t char_val_len);

#endif /* BLE_SERVICE_FTP_H_ */
