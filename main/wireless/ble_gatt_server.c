#include "ble_interface.h"

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_bt.h"

#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"

#include "ble_common.h"
#include "ble_gap.h"

#include "ble_services/ble_service_wifi_config.h"

#define TAG "GATT Server"

#define PROFILE_NUM                 1
#define PROFILE_APP_IDX             0
#define ESP_APP_ID                  0x55
// #define SAMPLE_DEVICE_NAME          "ESP32 Test Device"
#define SVC_INST_ID                 0

/* The max length of characteristic value. When the gatt client write or prepare write, 
*  the data length must be less than GATTS_DEMO_CHAR_VAL_LEN_MAX. 
*/
#define GATTS_DEMO_CHAR_VAL_LEN_MAX 500
#define PREPARE_BUF_MAX_SIZE        1024

#define ADV_CONFIG_FLAG             (1 << 0)
#define SCAN_RSP_CONFIG_FLAG        (1 << 1)

static uint8_t adv_config_done       = 0;


typedef struct {
    uint8_t                 *prepare_buf;
    int                     prepare_len;
} prepare_type_env_t;

static prepare_type_env_t prepare_write_env;

struct gatts_profile_inst {
    esp_gatts_cb_t gatts_cb;
    uint16_t gatts_if;
    uint16_t app_id;
    uint16_t conn_id;
    uint16_t service_handle;
    esp_gatt_srvc_id_t service_id;
    uint16_t char_handle;
    esp_bt_uuid_t char_uuid;
    esp_gatt_perm_t perm;
    esp_gatt_char_prop_t property;
    uint16_t descr_handle;
    esp_bt_uuid_t descr_uuid;
};

static esp_gatt_rsp_t rsp;


static void gatts_profile_event_handler(esp_gatts_cb_event_t event,
					esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);

/* One gatt-based profile one app_id and one gatts_if, this array will store the gatts_if returned by ESP_GATTS_REG_EVT */
static struct gatts_profile_inst bt_profile_tab[PROFILE_NUM] = {
    [PROFILE_APP_IDX] = {
        .gatts_cb = gatts_profile_event_handler,
        .gatts_if = ESP_GATT_IF_NONE,       /* Not get the gatt_if, so initial is ESP_GATT_IF_NONE */
    },
};

void example_prepare_write_event_env(esp_gatt_if_t gatts_if, prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param)
{
    ESP_LOGI(TAG, "prepare write, handle = %d, value len = %d", param->write.handle, param->write.len);
    esp_gatt_status_t status = ESP_GATT_OK;
    if (prepare_write_env->prepare_buf == NULL) {
        prepare_write_env->prepare_buf = (uint8_t *)malloc(PREPARE_BUF_MAX_SIZE * sizeof(uint8_t));
        prepare_write_env->prepare_len = 0;
        if (prepare_write_env->prepare_buf == NULL) {
            ESP_LOGE(TAG, "%s, Gatt_server prep no mem", __func__);
            status = ESP_GATT_NO_RESOURCES;
        }
    } else {
        if(param->write.offset > PREPARE_BUF_MAX_SIZE) {
            status = ESP_GATT_INVALID_OFFSET;
        } else if ((param->write.offset + param->write.len) > PREPARE_BUF_MAX_SIZE) {
            status = ESP_GATT_INVALID_ATTR_LEN;
        }
    }
    /*send response when param->write.need_rsp is true */
    if (param->write.need_rsp){
        esp_gatt_rsp_t *gatt_rsp = (esp_gatt_rsp_t *)malloc(sizeof(esp_gatt_rsp_t));
        if (gatt_rsp != NULL){
            gatt_rsp->attr_value.len = param->write.len;
            gatt_rsp->attr_value.handle = param->write.handle;
            gatt_rsp->attr_value.offset = param->write.offset;
            gatt_rsp->attr_value.auth_req = ESP_GATT_AUTH_REQ_NONE;
            memcpy(gatt_rsp->attr_value.value, param->write.value, param->write.len);
            esp_err_t response_err = esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, status, gatt_rsp);
            if (response_err != ESP_OK){
               ESP_LOGE(TAG, "Send response error");
            }
            free(gatt_rsp);
        }else{
            ESP_LOGE(TAG, "%s, malloc failed", __func__);
        }
    }
    if (status != ESP_GATT_OK){
        return;
    }
    memcpy(prepare_write_env->prepare_buf + param->write.offset,
           param->write.value,
           param->write.len);
    prepare_write_env->prepare_len += param->write.len;

}

void example_exec_write_event_env(prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param){
    if (param->exec_write.exec_write_flag == ESP_GATT_PREP_WRITE_EXEC && prepare_write_env->prepare_buf){
        esp_log_buffer_hex(TAG, prepare_write_env->prepare_buf, prepare_write_env->prepare_len);
    }else{
        ESP_LOGI(TAG,"ESP_GATT_PREP_WRITE_CANCEL");
    }
    if (prepare_write_env->prepare_buf) {
        free(prepare_write_env->prepare_buf);
        prepare_write_env->prepare_buf = NULL;
    }
    prepare_write_env->prepare_len = 0;
}

static void gatts_profile_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    switch (event) {
        case ESP_GATTS_REG_EVT:{
            ble_gap_setAdvertisingData();
            
            /*
             *  Add calls to register new attribute tables here
             *  Template:
             *  esp_err_t create_attr_ret = esp_ble_gatts_create_attr_tab(#SERVICE GATT DB#, gatts_if, #SERVICE MAX INDEX#, SVC_INST_ID);
             *  if (create_attr_ret){
             *      ESP_LOGE(TAG, "create attr table failed, error code = %x", create_attr_ret);
             *  }
             */
            
            esp_err_t create_attr_ret = esp_ble_gatts_create_attr_tab(wifi_serv_gatt_db, gatts_if, WIFI_NB, SVC_INST_ID);
            if (create_attr_ret){
                ESP_LOGE(TAG, "create attr table failed, error code = %x", create_attr_ret);
            }

            /* End calls to register external service attribute tables */
        }
       	    break;
        case ESP_GATTS_READ_EVT:
            ESP_LOGI(TAG, "ESP_GATTS_READ_EVT");
            
            memset(&rsp, 0, sizeof(esp_gatt_rsp_t));
            rsp.attr_value.handle = param->read.handle;

            esp_gatt_status_t status = ESP_GATT_READ_NOT_PERMIT;
            int               attrIndex;

            /*
             *  Add calls to handle read events for each external service here
             *  Template:
             *  if( (attrIndex = getAttributeIndexBy#SERVICE#Handle(param->read.handle)) < #SERVICE MAX INDEX# )
             *  {
             *      ESP_LOGI(TAG, "#SERVICE# READ");
             *      handle#SERVICE#ReadEvent(attrIndex, param, &rsp);
             *      status = ESP_GATT_OK;
             *  }
             */

            if( (attrIndex = getAttributeIndexByWifiHandle(param->read.handle)) < WIFI_NB )
            {
            	ESP_LOGI(TAG,"WIFI READ");
            	handleWifiReadEvent(attrIndex, param, &rsp);
            	status = ESP_GATT_OK;
            }

            /* END read handler calls for external services */
            
            if (param->read.need_rsp){
                esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id, ESP_GATT_OK, &rsp);
            }
       	    break;
        case ESP_GATTS_WRITE_EVT:
            if (!param->write.is_prep){
                // the data length of gattc write  must be less than GATTS_DEMO_CHAR_VAL_LEN_MAX.
                ESP_LOGI(TAG, "GATT_WRITE_EVT, handle = %d, value len = %d, value :", param->write.handle, param->write.len);
                esp_log_buffer_hex(TAG, param->write.value, param->write.len);

                /*
                *  Add calls to handle read events for each external service here
                *  Template:
                *  if( (attrIndex = getAttributeIndexBy#SERVICE#Handle(param->read.handle)) < #SERVICE MAX INDEX# )
                *  {
                *      ESP_LOGI(TAG, "#SERVICE# READ");
                *      handle#SERVICE#ReadEvent(attrIndex, param, &rsp);
                *      status = ESP_GATT_OK;
                *  }
                */

                if( (attrIndex = getAttributeIndexByWifiHandle(param->read.handle)) < WIFI_NB )
                {
                    ESP_LOGI(TAG,"WIFI READ");
                    //handleWifiWriteEvent(attrIndex, param, &rsp);
                    status = ESP_GATT_OK;
                }

                /* END read handler calls for external services */

                /* send response when param->write.need_rsp is true*/
                if (param->write.need_rsp){
                    esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OK, NULL);
                }
            }else{
                /* handle prepare write */
                example_prepare_write_event_env(gatts_if, &prepare_write_env, param);
            }
      	    break;
        case ESP_GATTS_EXEC_WRITE_EVT: 
            // the length of gattc prapare write data must be less than GATTS_DEMO_CHAR_VAL_LEN_MAX. 
            ESP_LOGI(TAG, "ESP_GATTS_EXEC_WRITE_EVT");
            example_exec_write_event_env(&prepare_write_env, param);
            break;
        case ESP_GATTS_MTU_EVT:
            ESP_LOGI(TAG, "ESP_GATTS_MTU_EVT, MTU %d", param->mtu.mtu);
            break;
        case ESP_GATTS_CONF_EVT:
            ESP_LOGI(TAG, "ESP_GATTS_CONF_EVT, status = %d", param->conf.status);
            break;
        case ESP_GATTS_START_EVT:
            ESP_LOGI(TAG, "SERVICE_START_EVT, status %d, service_handle %d", param->start.status, param->start.service_handle);
            break;
        case ESP_GATTS_CONNECT_EVT:
            ESP_LOGI(TAG, "ESP_GATTS_CONNECT_EVT, conn_id = %d", param->connect.conn_id);
            esp_log_buffer_hex(TAG, param->connect.remote_bda, 6);
            esp_ble_conn_update_params_t conn_params = {0};
            memcpy(conn_params.bda, param->connect.remote_bda, sizeof(esp_bd_addr_t));
            /* For the IOS system, please reference the apple official documents about the ble connection parameters restrictions. */
            conn_params.latency = 0;
            conn_params.max_int = 0x20;    // max_int = 0x20*1.25ms = 40ms
            conn_params.min_int = 0x10;    // min_int = 0x10*1.25ms = 20ms
            conn_params.timeout = 400;    // timeout = 400*10ms = 4000ms
            //start sent the update connection parameters to the peer device.
            esp_ble_gap_update_conn_params(&conn_params);
            break;
        case ESP_GATTS_DISCONNECT_EVT:
            ESP_LOGI(TAG, "ESP_GATTS_DISCONNECT_EVT, reason = %d", param->disconnect.reason);
            ble_gap_startAdvertising();
            break;
        case ESP_GATTS_CREAT_ATTR_TAB_EVT:{
            if (param->add_attr_tab.status != ESP_GATT_OK)
            {
                ESP_LOGE(TAG,"create attribute table failed, error code=0x%x", param->add_attr_tab.status);
            }

            /*
             *  Add calls to start external services
             *  Template:
             *  else if(param->add_attr_tab.svc_uuid.uuid.uuid16 == #SERVICE UUID#)
             *  {
             *      if( param->add_attr_tab.num_handle != #SERVICE MAX INDEX#)
             *      {
             *          ESP_LOGE(TAG,"create attribute table abnormall, num_handle (%d) isn't equal to #SERVICE MAX INDEX#(%d)", param->add_attr_tab.num_handle, #SERVICE MAX INDEX#);
             *      }
             *      else
             *      {
             *          ESP_LOGI(TAG,"create attribute table successfully, the number handle = %d\n",param->add_attr_tab.num_handle);
             *          memcpy(#SERVICE#_handle_table, param->add_attr_tab.handles, sizeof(#SERVICE#_handle_Table));
             *          esp_ble_gatts_start_service(#SERVICE#_handle_table[#SERVICE INDEX 0#]);
             *      }
             *  }
             * 
             */

            else if(param->add_attr_tab.svc_uuid.uuid.uuid16 == WIFI_SERV_uuid)
            {
				if(param->add_attr_tab.num_handle != WIFI_NB)
				{
					ESP_LOGE(TAG,"create attribute table abnormally, num_handle (%d) isn't equal to INFO_NB(%d)", param->add_attr_tab.num_handle, WIFI_NB);
				}
				else
				{
					ESP_LOGI(TAG,"create attribute table successfully, the number handle = %d\n",param->add_attr_tab.num_handle);
					memcpy(wifi_handle_table, param->add_attr_tab.handles, sizeof(wifi_handle_table));
					esp_ble_gatts_start_service(wifi_handle_table[WIFI_SERV]);
				}
            }

            /* END start external services */
            break;
        }
        case ESP_GATTS_STOP_EVT:
        case ESP_GATTS_OPEN_EVT:
        case ESP_GATTS_CANCEL_OPEN_EVT:
        case ESP_GATTS_CLOSE_EVT:
        case ESP_GATTS_LISTEN_EVT:
        case ESP_GATTS_CONGEST_EVT:
        case ESP_GATTS_UNREG_EVT:
        case ESP_GATTS_DELETE_EVT:
        default:
            break;
    }
}


static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{

    /* If event is register event, store the gatts_if for each profile */
    if (event == ESP_GATTS_REG_EVT) {
        if (param->reg.status == ESP_GATT_OK) {
            bt_profile_tab[PROFILE_APP_IDX].gatts_if = gatts_if;
        } else {
            ESP_LOGE(TAG, "reg app failed, app_id %04x, status %d",
                    param->reg.app_id,
                    param->reg.status);
            return;
        }
    }
    do {
        int idx;
        for (idx = 0; idx < PROFILE_NUM; idx++) {
            /* ESP_GATT_IF_NONE, not specify a certain gatt_if, need to call every profile cb function */
            if (gatts_if == ESP_GATT_IF_NONE || gatts_if == bt_profile_tab[idx].gatts_if) {
                if (bt_profile_tab[idx].gatts_cb) {
                    bt_profile_tab[idx].gatts_cb(event, gatts_if, param);
                }
            }
        }
    } while (0);
}

esp_err_t ble_gatt_server_init(void)
{
	esp_err_t err;

	ESP_LOGI(TAG,"Initializing Bluetooth Interface");

	err = esp_ble_gatts_register_callback(gatts_event_handler);
	if(err)
	{
		ESP_LOGE(TAG,"Gatts register failed with err: %x", err);
		return err;
	}

	err = esp_ble_gatts_app_register(ESP_APP_ID);
	if(err)
	{
		ESP_LOGE(TAG,"App register failed with err: %x", err);
		return err;
	}

    err = esp_ble_gatt_set_local_mtu(500);
    if (err)
    {
        ESP_LOGE(TAG,"set local  MTU failed, error code = %x", err);
    }

	ESP_LOGI(TAG,"Bluetooth Interface initialized successfully");

	return err == ESP_OK;
}



esp_err_t ble_gatt_server_deinit(void)
{
	esp_err_t err;
	err = ESP_OK;

	if(esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_ENABLED)
	{
		//esp_ble_gatts_app_unregister(//gattsif);
	}

	return err == ESP_OK;
}