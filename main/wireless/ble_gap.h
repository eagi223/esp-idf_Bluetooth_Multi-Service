#ifndef BLE_GAP_H_
	#define BLE_GAP_H_

	#include <stdbool.h>

	#define ADV_CONFIG_FLAG		    (1 << 0)
	#define SCAN_RSP_CONFIG_FLAG	(1 << 1)

	void ble_gap_setAdvertisingData(void);

	void ble_gap_startAdvertising(void);

	bool ble_gap_init(void);

	void ble_gap_deinit(void);


#endif /* BLE_GAP_H_ */