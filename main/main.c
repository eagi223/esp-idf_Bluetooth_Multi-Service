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
#include "main.h"
#include "wireless/ble_interface.h"

#define TAG "main"

#ifdef CONFIG_EXAMPLE_FS_TYPE_SPIFFS
// SPIFFS filesystem
#error "SPIFFS Filesystem not implemented!"
#elif defined CONFIG_EXAMPLE_FS_TYPE_FAT
// FAT file-system
#warning "FAT Filesystem Chosen"
#include "esp_vfs.h"
#include "esp_vfs_fat.h"
#include "esp_partition.h"
// Handle of the wear levelling library instance
static wl_handle_t s_wl_handle = WL_INVALID_HANDLE;  // needed by FAT filesystem

#elif defined CONFIG_EXAMPLE_FS_TYPE_LFS
// LittleFS Filesystem
#error "LittleFS Filesystem not implemented!"
#else
#warning "No filesystem Chosen!"
#endif

#ifdef CONFIG_EXAMPLE_ROFS_TYPE_ESPFS
#include "espfs.h"
#include "espfs_image.h"
#include "espfs_vfs.h"
#endif // CONFIG_EXAMPLE_ROFS_TYPE_ESPFS

static void mount_fs(void)
{
#ifdef CONFIG_EXAMPLE_ROFS_TYPE_ESPFS
	// Mount trivial read-only filesystem: espfs
	esp_vfs_espfs_conf_t espfs_vfs_conf = {
		.base_path = ESPFS_BASE_PATH,
	    .max_files = 5
	};
	//espFsInit((void*)(image_espfs_start)); -- next call will do this
	esp_vfs_espfs_register(&espfs_vfs_conf);
#endif // CONFIG_EXAMPLE_ROFS_TYPE_ESPFS

#ifndef CONFIG_EXAMPLE_FS_TYPE_NONE
	esp_err_t err = ESP_FAIL;
	int f_bsize=0, f_blocks=0, f_bfree=0;
    esp_partition_t * fs_partition = NULL;

    ESP_LOGI(TAG, "Mounting Filesystem");
    // To mount device we need name of device partition, define base_path
    // and allow format partition in case if it is new one and was not formated before

#ifdef CONFIG_EXAMPLE_FS_TYPE_SPIFFS
    // SPIFFS filesystem
#elif defined CONFIG_EXAMPLE_FS_TYPE_FAT
    // FAT file-system
    fs_partition = (esp_partition_t *)esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_FAT, FS_PART_NAME);
	if (fs_partition == NULL) {
		ESP_LOGE(TAG, "Filesystem partition not found!");
	}
    const esp_vfs_fat_mount_config_t mount_config = {
	    .max_files = 16,
	    .format_if_mount_failed = true,
	    .allocation_unit_size = 0 // will default to wear-leveling size.  Set CONFIG_WL_SECTOR_SIZE = 4096 in menuconfig for performance boost.
    };
    err = esp_vfs_fat_spiflash_mount(FS_BASE_PATH, FS_PART_NAME, &mount_config, &s_wl_handle);
    if (err != ESP_OK) {
    	ESP_LOGE(TAG, "Failed to mount FATFS (%s)", esp_err_to_name(err));
    	return;
    }
	FATFS *fatfs;
	DWORD fre_clust;
	FRESULT res = f_getfree(FS_BASE_PATH, &fre_clust, &fatfs);
	if (res != FR_OK) {
	    ESP_LOGE(TAG, "Failed to get FAT partition information (error: %i)", res);
	} else {
#if FF_MAX_SS == FF_MIN_SS
#define SECSIZE(fs) (FF_MIN_SS)
#else
#define SECSIZE(fs) ((fs)->ssize)
#endif
		f_bsize = fatfs->csize * SECSIZE(fatfs);
		f_blocks = fatfs->n_fatent - 2;
		f_bfree = fre_clust;
	}
	ESP_LOGI(TAG, "Mounted filesystem. Type: FAT");
#elif defined CONFIG_EXAMPLE_FS_TYPE_LFS
    // LittleFS Filesystem
#endif
	if (fs_partition != NULL)
	{
		ESP_LOGI(TAG, "Mounted on partition '%s' [size: %d; offset: 0x%6X; %s]", fs_partition->label, fs_partition->size, fs_partition->address, (fs_partition->encrypted)?"ENCRYPTED":"");
		if (err == ESP_OK) {
			ESP_LOGI(TAG, "----------------");
			ESP_LOGI(TAG, "Filesystem size: %d B", f_blocks * f_bsize);
			ESP_LOGI(TAG, "           Used: %d B", (f_blocks * f_bsize) - (f_bfree * f_bsize));
			ESP_LOGI(TAG, "           Free: %d B", f_bfree * f_bsize);
			ESP_LOGI(TAG, "----------------");
		}
	}
#endif //!CONFIG_EXAMPLE_FS_TYPE_NONE
}

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
	
    mount_fs();
    ble_interface_init();
	
    printf("\nReady\n");
    while(1)
    {
        ESP_LOGI( TAG, "main loop" );
        
        vTaskDelay(1000);
    }

}

