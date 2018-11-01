/* Factory模式
 * 用于初始化用户设置和配置网络，并更新至用户分区
 * */

#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "esp_wpa2.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "tcpip_adapter.h"
#include "nvs.h"
#include "wifi_cfg.h"
#include "led.h"



static const char *MAIN_TAG = "main";
extern EventGroupHandle_t wifi_event_group;
QueueHandle_t phoneIP_queue;

void app_main()
{
    // 初始化 NVS，并存储重启次数
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
    	ESP_LOGE(MAIN_TAG,"ESP_ERR_NVS_NO_FREE_PAGES");
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );
    nvs_handle NVS_handle;
    err = nvs_open("user_data", NVS_READWRITE, &NVS_handle);
    if(err != ESP_OK){
    	ESP_LOGE(MAIN_TAG,"Error (%d) opening NVS handle!\n", err);
    }else{
    	uint32_t restart_counter = 0; // value will default to 0, if not set yet in NVS
		err = nvs_get_u32(NVS_handle, "restart_counter", &restart_counter);
		switch (err) {
			case ESP_OK:
				ESP_LOGI(MAIN_TAG,"Restart counter = %d\n", restart_counter);
				break;
			case ESP_ERR_NVS_NOT_FOUND:
				ESP_LOGI(MAIN_TAG,"The value is not initialized yet!\n");
				break;
			default :
				ESP_LOGW(MAIN_TAG,"Error (%d) reading!\n", err);
		}
		// Write
		ESP_LOGI(MAIN_TAG,"Updating restart counter in NVS ... ");
		restart_counter++;
		err = nvs_set_u32(NVS_handle, "restart_counter", restart_counter);
		if (err != ESP_OK){
			ESP_LOGW(MAIN_TAG,"Error set restart_counter!\n");
		}else{
			ESP_LOGI(MAIN_TAG,"set restart_counter ok.\n");
		}

		// Commit written value.
		// After setting any values, nvs_commit() must be called to ensure changes are written
		// to flash storage. Implementations may write to storage at other times,
		// but this is not guaranteed.
		ESP_LOGI(MAIN_TAG,"Committing updates in NVS ... ");
		err = nvs_commit(NVS_handle);
		if (err != ESP_OK){
			ESP_LOGW(MAIN_TAG,"Error commit restart_counter!\n");
		}else{
			ESP_LOGI(MAIN_TAG,"commit OK!");
		}
    }
    phoneIP_queue = xQueueGenericCreate(4,1,queueQUEUE_TYPE_BASE);	//消息队列来获取手机IP
    init_wifi();
    uint8_t phone_ip[4] = { 0 };
    for(int i=0;i<4;i++){
    	xQueueReceive(phoneIP_queue,(phone_ip+i),portMAX_DELAY);
    	printf("%d",phone_ip[i]);
    }







	nvs_close(NVS_handle);

}




