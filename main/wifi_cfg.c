/*
 * wifi_cfg.c
 *
 *  Created on: 2018年11月1日
 *      Author: zxd
 */
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "esp_wifi.h"
#include "esp_wpa2.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "tcpip_adapter.h"
#include "esp_smartconfig.h"
#include "wifi_cfg.h"
#include "led.h"

static const int CONNECTED_BIT = BIT0;
static const int ESPTOUCH_DONE_BIT = BIT1;



static const char *wifi_TAG = "wifi";
static esp_err_t event_handler(void *ctx, system_event_t *event);
static void smart_cfg_task(void * parm);
static void sc_callback(smartconfig_status_t status, void *pdata);

extern QueueHandle_t phoneIP_queue;
EventGroupHandle_t wifi_event_group;

void init_wifi(){
	/**
	 * 初始化wifi并进入smart config模式
	 */
	tcpip_adapter_init();
	wifi_event_group = xEventGroupCreate();
	xTaskCreate(led_task,"led task",2048,NULL,3,NULL);
	ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

	ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
	ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
	ESP_ERROR_CHECK( esp_wifi_start() );
}


static void smart_cfg_task(void * parm)
{
    EventBits_t uxBits;
    ESP_ERROR_CHECK( esp_smartconfig_set_type(SC_TYPE_ESPTOUCH) );
    ESP_ERROR_CHECK( esp_smartconfig_start(sc_callback) );
    while (1) {
        uxBits = xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT | ESPTOUCH_DONE_BIT, true, false, portMAX_DELAY);
        if(uxBits & CONNECTED_BIT) {
            ESP_LOGI(wifi_TAG, "WiFi Connected to ap");
        }
        if(uxBits & ESPTOUCH_DONE_BIT) {
            ESP_LOGI(wifi_TAG, "smartconfig over");
            esp_smartconfig_stop();
            vTaskDelete(NULL);
        }
    }
}

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch(event->event_id) {
    case SYSTEM_EVENT_STA_START:
    	xEventGroupSetBits(wifi_event_group, WIFI_INIT);
        xTaskCreate(smart_cfg_task, "smartconfig_example_task", 4096, NULL, 3, NULL);
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
    	xEventGroupSetBits(wifi_event_group, GOT_IP);
        xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
        xEventGroupClearBits(wifi_event_group, STA_DISCONNECTED);
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        esp_wifi_connect();
        xEventGroupSetBits(wifi_event_group, STA_DISCONNECTED);
        xEventGroupClearBits(wifi_event_group, GOT_IP);
        xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
        break;
    default:
        break;
    }
    return ESP_OK;
}

static void sc_callback(smartconfig_status_t status, void *pdata)
{
    switch (status) {
        case SC_STATUS_WAIT:
            ESP_LOGI(wifi_TAG, "SC_STATUS_WAIT");
            break;
        case SC_STATUS_FIND_CHANNEL:
            ESP_LOGI(wifi_TAG, "SC_STATUS_FINDING_CHANNEL");
            break;
        case SC_STATUS_GETTING_SSID_PSWD:
            ESP_LOGI(wifi_TAG, "SC_STATUS_GETTING_SSID_PSWD");
            break;
        case SC_STATUS_LINK:
            ESP_LOGI(wifi_TAG, "SC_STATUS_LINK");
            wifi_config_t *wifi_config = pdata;
            ESP_LOGI(wifi_TAG, "SSID:%s", wifi_config->sta.ssid);
            ESP_LOGI(wifi_TAG, "PASSWORD:%s", wifi_config->sta.password);
            ESP_ERROR_CHECK( esp_wifi_disconnect() );
            ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_STA, wifi_config) );
            ESP_ERROR_CHECK( esp_wifi_connect() );
            break;
        case SC_STATUS_LINK_OVER:
            ESP_LOGI(wifi_TAG, "SC_STATUS_LINK_OVER");
            if (pdata != NULL) {
                uint8_t phone_ip[4] = { 0 };
                memcpy(phone_ip, (uint8_t* )pdata, 4);
                BaseType_t err;
                for (int i = 0;i<4;i++)
                {
                	err = xQueueSend(phoneIP_queue, (phone_ip+i), 0);
                	if (err != pdPASS)
                	{
                		ESP_LOGW(wifi_TAG,"phoneIP send to queue failed.");
                	}
                }
                ESP_LOGI(wifi_TAG, "Phone ip: %d.%d.%d.%d\n", phone_ip[0], phone_ip[1], phone_ip[2], phone_ip[3]);
            }
            xEventGroupSetBits(wifi_event_group, ESPTOUCH_DONE_BIT);
            break;
        default:
            break;
    }
}



