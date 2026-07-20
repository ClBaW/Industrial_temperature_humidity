#ifndef OTA_STATEMACHINE_H
#define OTA_STATEMACHINE_H

#include <string.h>
#include <stdio.h>

#include "./SYSTEM/usart/usart.h"
#include "./SYSTEM/delay/delay.h"
#include "./BSP/KEY/key.h"

#include "ota_25q128.h"
#include"ota_24c02.h"
#include"ota_wifi.h"
#include "ota_wifi_usart.h"

#define OTA_RECV_TIMEOUT 5000
extern char ota_progress_str[32];  /* 屏幕升级提示(UI显示) */
  /* 收流超时: 10秒无新数据判失败 */

typedef enum
{
    OTA_STATE_IDLE = 0,     /* 待机: 通过lvgl触发升级状态 */
    OTA_STATE_WIFI_CONNECT, /* 连接WiFi */
    OTA_STATE_TCP_CONNECT,  /* 连接TCP服务器 */
    OTA_STATE_PASSTHROUGH,  /* 进入透传模式 */
    OTA_STATE_RECEIVE,      /* 接收固件并写入W25Q128 */
    OTA_STATE_FINISH,       /* 下载完成: 写24C02标志并复位 */
    OTA_STATE_ERROR         /* 出错: 打印并回待机 */
} ota_state_t;

extern ota_state_t ota_state;             /* 当前状态 */
extern uint32_t recv_count;               /* 已接收字节数 */
extern uint16_t page_nb;                  /* 页号(0~3840, 960KB/256B) */
extern uint8_t page_buf[256];             /* 页缓冲: 攒满256B写一页 */
extern uint16_t page_cnt;                 /* 页缓冲内字节数 */
extern uint32_t last_tick;                /* 最后收到数据的时间(HAL_GetTick) */
extern uint8_t recv_buf[512];             /* 收流缓冲(stream_read用, 必须大于单段257B) */
extern char ip_buf[32];                   /* IP地址显示缓冲 */
extern OTA_FlagTypeDef ota_flag_info;     /* 升级标志(结构体与Bootloader工程一致) */

void ota_Statemachine_start(void);

void ota_Statemachine_init(void);

void ota_Statemachine_while(void);

void ota_Statemachine_evet(uint8_t *data, uint16_t datalen);


#endif // OTA_STATEMACHINE_H
