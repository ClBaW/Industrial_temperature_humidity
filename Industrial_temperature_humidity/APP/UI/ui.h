#ifndef UI_H
#define UI_H
    
#include <string.h>
#include "lvgl.h"

#include "FreeRTOS.h"
#include "task.h"

#include "modbus.h"               
#include "ota_wifi.h"            
#include "ota_Statemachine.h"     
#include "ota_24c02.h"            
#include "./BSP/LED/led.h"   
#include "./BSP/BEEP/beep.h"

/* ---- 设备信息: 数值自己改, 单位 KB, WiFi页显示? ---- */
#define RAM_INT   192      /* 内部 RAM: STM32F407 192KB */
#define RAM_EXT   1024     /* 外部 RAM: IS62WV51216 1MB */
#define FLASH_INT 1024     /* 内部 Flash: 1MB */
#define FLASH_EXT 16    /* 外部 Flash: W25Q128 16MB */

/* --- 文字颜色 --- */
#define C_BLUE        lv_palette_darken(LV_PALETTE_BLUE, 1)
#define C_GREEN       lv_palette_darken(LV_PALETTE_GREEN, 1)
#define C_RED         lv_palette_darken(LV_PALETTE_RED, 2)
#define C_Light_GREY  lv_palette_main(LV_PALETTE_GREY)

/* --- 阈值: 24C02 偏移 0x80 起 8 字节(OTA 区 0~79) --- */
#define THR_ADDR 0x80

/* 中文字库 */
LV_FONT_DECLARE(cn_font16);   /* 16px 正文 */
LV_FONT_DECLARE(cn_font36);   /* 36px 大数字? */
LV_FONT_DECLARE(cn_font48);   /* 48px 大数字?(首页温湿度?) */

void ui_init(void);

/* --- WiFi/服务器设置(OTA 状态机取用) --- */
char *data_wifi_ssid_get(void);
char *data_wifi_pwd_get(void);
char *data_wifi_srv_ip_get(void);
char *data_wifi_srv_port_get(void);

#endif /* UI_H */
