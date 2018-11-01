/*
 * wifi_cfg.h
 *
 *  Created on: 2018年11月1日
 *      Author: zxd
 * EventGroupHandle_t wifi_event_group 使用说明：
 * BIT0，BIT1为局部参数不使用
 * BIT2：wifi开始初始化，由led.c接收并清零
 * BIT3：smart_cfg_task开始运行，由led.c接收并清零
 * BIT4：SC_STATUS_GETTING_SSID_PSWD，由led.c接收并清零
 * BIT5：SC_STATUS_LINK_OVER，由led.c接收并清零
 * BIT6：SYSTEM_EVENT_STA_GOT_IP，由SYSTEM_EVENT_STA_DISCONNECTED清零
 * BIT7：SYSTEM_EVENT_STA_DISCONNECTED，由SYSTEM_EVENT_STA_GOT_IP清零
 *
 */

#ifndef MAIN_WIFI_CFG_H_
#define MAIN_WIFI_CFG_H_

void init_wifi();
#define WIFI_INIT 			BIT2
#define SC_START 			BIT3
#define SC_GET_WIFIID_PWD 	BIT4
#define SC_LINK_OVER 		BIT5
#define GOT_IP 				BIT6
#define STA_DISCONNECTED	BIT7

#endif /* MAIN_WIFI_CFG_H_ */
