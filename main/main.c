#include "freertos/FreeRTOS.h"

#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"
#include "driver/gpio.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "wireless/ble_interface.h"

#define TAG "main"

void app_main(void)
{
    esp_err_t err;

    // Initialize NVS.
    err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );

    ble_interface_init();;

    while(1)
    {
        ESP_LOGI( TAG, "main loop" );
        
        vTaskDelay(10);
    }

}

