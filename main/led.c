/*
 * led.c
 *
 *  Created on: 2018年11月1日
 *      Author: zxd
 */
#include <string.h>
#include <stdlib.h>
#include "led.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_event_loop.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_err.h"
#include "driver/ledc.h"
#include "driver/gpio.h"
#include "wifi_cfg.h"

/*
 * LED 相关配置信息
 */
#define LEDC_LS_TIMER          LEDC_TIMER_1
#define LEDC_LS_MODE           LEDC_LOW_SPEED_MODE
#define LEDC_LS_CH0_GPIO       (14)
#define LEDC_LS_CH0_CHANNEL    LEDC_CHANNEL_2
#define LEDC_TEST_DUTY		   (4000)
#define LEDC_TEST_FADE_TIME    (1000)

/*
 * GPIO配置信息，用于禁止IO12，13，15的输出
 */
#define GPIO_OUTPUT_PIN_SEL  ((1ULL<<12) | (1ULL<<13) | (1ULL<<15))

static const char *led_TAG = "led";

extern EventGroupHandle_t wifi_event_group;

void led_task(void * pvParameters)
{
	uint16_t fade_wait_time = 0;	//呼吸灯模式等待时间
	EventBits_t system_state;
	ledc_timer_config_t ledc_timer = {
	        .duty_resolution = LEDC_TIMER_13_BIT, // resolution of PWM duty
	        .freq_hz = 5000,                      // frequency of PWM signal
	        .speed_mode = LEDC_LS_MODE,           // timer mode
	        .timer_num = LEDC_LS_TIMER            // timer index
	};
    ledc_timer_config(&ledc_timer);
    ledc_channel_config_t ledc_channel={
    		.channel    = LEDC_LS_CH0_CHANNEL,
    		.duty       = 0,
    		.gpio_num   = LEDC_LS_CH0_GPIO,
    		.speed_mode = LEDC_LS_MODE,
    		.timer_sel  = LEDC_LS_TIMER
    };
    ledc_channel_config (&ledc_channel);
    //ledc_fade_func_install(0);

    //禁用12,13,15引脚的输出
    gpio_config_t io_conf;
    io_conf.intr_type=GPIO_PIN_INTR_DISABLE;
    io_conf.mode=GPIO_MODE_DEF_DISABLE;
    io_conf.pin_bit_mask=GPIO_OUTPUT_PIN_SEL;
    io_conf.pull_down_en=0;
    io_conf.pull_up_en=0;
    gpio_config(&io_conf);
    ESP_LOGI(led_TAG,"LED start");
    ledc_stop(LEDC_LS_MODE,LEDC_LS_CH0_CHANNEL,1);		//点亮led

    /****  等待smart config启动  *****/
    xEventGroupWaitBits(wifi_event_group,SC_START,pdTRUE,pdTRUE,portMAX_DELAY);		//等待smart config启动
    ESP_ERROR_CHECK(ledc_fade_func_install(0));		//注册呼吸灯
    int led_cycle = 0;
    while(1){
    	system_state = xEventGroupGetBits(wifi_event_group);
    	if(((system_state & SC_LINK_OVER) == SC_LINK_OVER) || ((system_state & GOT_IP) == GOT_IP)){
    		break;
    	}
    	if(led_cycle == 0){
    		led_cycle = 18;
    		ESP_ERROR_CHECK(ledc_set_fade_with_time(ledc_channel.speed_mode,
												ledc_channel.channel, LEDC_TEST_DUTY, LEDC_TEST_FADE_TIME));
    		ESP_ERROR_CHECK(ledc_fade_start(ledc_channel.speed_mode,
												ledc_channel.channel, LEDC_FADE_NO_WAIT ));
    	}else if(led_cycle == 9){
    		ESP_ERROR_CHECK(ledc_set_fade_with_time(ledc_channel.speed_mode,
												ledc_channel.channel, 0, LEDC_TEST_FADE_TIME));
    		ESP_ERROR_CHECK(ledc_fade_start(ledc_channel.speed_mode,
												ledc_channel.channel, LEDC_FADE_NO_WAIT ));
    	}
    	vTaskDelay(100 / portTICK_PERIOD_MS);
    	led_cycle--;
    }
    ledc_fade_func_uninstall();
    /***********  连接完成后快速闪烁5次  ************/
    for (int i=0;i<5;i++){
		ledc_stop(LEDC_LS_MODE,LEDC_LS_CH0_CHANNEL,1);
		vTaskDelay(200 / portTICK_PERIOD_MS);
		ledc_stop(LEDC_LS_MODE,LEDC_LS_CH0_CHANNEL,0);
		vTaskDelay(200 / portTICK_PERIOD_MS);
	}

	while (1)
	{
		ESP_LOGI(led_TAG,"LED RUN");
		/*********  下面是高低电平闪烁模式  ***********/
		for (int i=0;i<5;i++){
			ledc_stop(LEDC_LS_MODE,LEDC_LS_CH0_CHANNEL,1);
			vTaskDelay(2000 / portTICK_PERIOD_MS);
			ledc_stop(LEDC_LS_MODE,LEDC_LS_CH0_CHANNEL,0);
			vTaskDelay(2000 / portTICK_PERIOD_MS);
		}
	}
	vTaskDelete(NULL);

}



