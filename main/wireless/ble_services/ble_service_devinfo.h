
#ifndef DEVINFO_SERVICE_H_
#define DEVINFO_SERVICE_H_
#include <stdint.h>

#include "esp_gatt_defs.h"
#include "esp_gatts_api.h"

enum
{
	// Service index
	DEVINFO_SERV,

	// System ID Declaration
	DEVINFO_SYSTEM_ID_CHAR,
	// System ID Value
	DEVINFO_SYSTEM_ID_VAL,

	// Model Number String Declaration
	DEVINFO_MODEL_NUMBER_STR_CHAR,
	// Model Number String Value
	DEVINFO_MODEL_NUMBER_STR_VAL,

	// Serial Number String Declaration
	DEVINFO_SERIAL_NUMBER_STR_CHAR,
	// Serial Number String Value
	DEVINFO_SERIAL_NUMBER_STR_VAL,

	// Firmware Revision String Declaration
	DEVINFO_FW_VERSION_STR_CHAR,
	// Firmware Revision String Value
	DEVINFO_FW_VERSION_STR_VAL,

	// Hardware Revision String Declaration
	DEVINFO_HW_VERSION_STR_CHAR,
	// Hardware Revision String Value
	DEVINFO_HW_VERSION_STR_VAL,

	// Software Revision String Declaration
	DEVINFO_SW_VERSION_STR_CHAR,
	// Software Revision String Value
	DEVINFO_SW_VERSION_STR_VAL,

	// Manufacturer Name String Declaration
	DEVINFO_MANU_NAME_CHAR,
	// Manufacturer Name String Value
	DEVINFO_MANU_NAME_VAL,

	// IEEE 11073-20601 Regulatory Certification Data List Declaration
	DEVINFO_IEEE_DATA_CHAR,
	// IEEE 11073-20601 Regulatory Certification Data List Value
	DEVINFO_IEEE_DATA_VAL,

	// PnP ID Declaration
	DEVINFO_PNP_ID_CHAR,
	// PnP ID Value
	DEVINFO_PNP_ID_VAL,

	DEVINFO_SERV_NUM_ATTR,
};

uint16_t devinfo_handle_table[DEVINFO_SERV_NUM_ATTR];

extern const uint16_t uuid_DEVINFO_SERV;
extern const esp_gatts_attr_db_t devinfo_serv_gatt_db[DEVINFO_SERV_NUM_ATTR];

#endif /* DEVINFO_SERVICE_H_ */
