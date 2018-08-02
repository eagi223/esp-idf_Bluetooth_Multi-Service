#ifndef BLE_GATT_SERVER_H_
    #define BLE_GATT_SERVER_H_

    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>

    #include "esp_err.h"

    esp_err_t ble_gatt_server_init();
    esp_err_t ble_gatt_server_deinit();

#endif /* BLE_GATT_SERVER_H_ */