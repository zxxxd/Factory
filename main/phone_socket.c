/*
 * phone_socket.c
 *
 *  Created on: 2018年11月2日
 *      Author: zxd
 */
#include "phone_socket.h"
#include <string.h>
#include "stdlib.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "cJSON.h"

#define PORT 8080

extern QueueHandle_t phoneIP_queue;
static const char *SOCKET_TAG = "socket";
static const uint8_t phoneIP_TEST[4] = {192,168,18,119};

esp_err_t IPv4_numtostr(uint8_t *ip_num, char *ip_str);		//把数字IP转换为字符串

/*
	void HexToStr(uint16_t number, char *str_out)；
	功能：把数字转为对应的字符串
	输入：number，输入的数字。
		  *str_out，以指针形式输出转换后的字符串
	输出：无
*/
void HexToStr(uint16_t number, char *str_out)
{
	char num_str[17] = "0123456789ABCDEF";
	char n;
	uint16_t i;
	i = number;
	i = (i & 0xf000) >> 12;
	*str_out = num_str[i];
	i = number;
	i = (i & 0x0f00) >> 8;
	*(str_out+1) = num_str[i];
	i = number;
	i = (i & 0x00f0) >> 4;
	*(str_out+2) = num_str[i];
	i = number;
	i = i & 0x000f;
	*(str_out+3) = num_str[i];
}

void get_userID(){
	int sockfd;						// 客户套接字标识符
	int len;						// 客户消息长度
	struct sockaddr_in addr;		// 客户套接字地址
	int newsockfd;
	char buf[256];//要发送的消息
	int len2;
	char rebuf[256];
	int err = 0;
	char IP_str[16];
	memset(IP_str,'/0',16);
	/**********  从smart config获取手机ip  *************/
	uint8_t phone_ip[4] = { 0 };
	for(int i=0;i<4;i++){
		xQueueReceive(phoneIP_queue,(phone_ip+i),portMAX_DELAY);
		printf("%d.",phone_ip[i]);
	}
	ESP_ERROR_CHECK(IPv4_numtostr(phoneIP_TEST,IP_str));
	ESP_LOGI(SOCKET_TAG,"set IP:%s",IP_str);
	/***************  与手机建立socket连接来获取商户数据  ****************/
	sockfd = socket(AF_INET,SOCK_STREAM, 0);	// 创建套接字
	addr.sin_family = AF_INET;			// 客户端套接字地址中的域
	addr.sin_port = PORT;								// 客户端套接字端口
	err = inet_pton(AF_INET, IP_str,&addr.sin_addr);
	printf("err = %d\n", err);
	len = sizeof(addr);
	newsockfd = connect(sockfd, (struct sockaddr *) &addr, len);	//发送连接服务器的请求
	while(1){
		if (newsockfd == -1) {
			perror("连接失败");
		}
		else
		{
			len2=sizeof(buf);
			err = 0;
			while(1){
				memset(buf,NULL,256);
				HexToStr(err, buf);
				printf("发送的数据:%s\n", buf);
				err++;
				send(sockfd,buf,len2,0); //发送消息
				if(recv(sockfd,rebuf,256,0)>0)//接收新消息
				{//rebuf[sizeof(rebuf)+1]='\0';
					printf("收到服务器消息:\n%s\n",rebuf);//输出到终端
				}
				vTaskDelay(1000 / portTICK_PERIOD_MS);
			}
		}
	}
	close(sockfd);				// 关闭连接

}

esp_err_t IPv4_numtostr(uint8_t *ip_num, char *ip_str)
{
	int err=0;
	err = sprintf(ip_str, "%u.%u.%u.%u",*ip_num,*(ip_num+1),*(ip_num+2),*(ip_num+3));
	if(err<0){
		printf("IPv4_numtostr err = %d",err);
		return ESP_FAIL;
	}else{
		return ESP_OK;
	}

}




