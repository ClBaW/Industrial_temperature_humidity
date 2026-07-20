#ifndef OTA_WIFI_H
#define OTA_WIFI_H

#include "stdio.h"
#include <string.h>

#include "./SYSTEM/usart/usart.h"
#include "./SYSTEM/delay/delay.h"

#include "ota_25q128.h"
#include "ota_24c02.h"
#include "ota_wifi_usart.h"

/* 引脚定义 根据实际引脚修改 */
#define ota_esp8266_RST_GPIO_PORT         GPIOF
#define ota_esp8266_RST_GPIO_PIN          GPIO_PIN_6
#define ota_esp8266_RST_GPIO_CLK_ENABLE() do{ __HAL_RCC_GPIOF_CLK_ENABLE(); }while(0)  /* PF时钟使能 */

/* IO操作 */
#define ota_8266D_RST(x)  do{ x ? \
                                HAL_GPIO_WritePin(ota_esp8266_RST_GPIO_PORT, ota_esp8266_RST_GPIO_PIN, GPIO_PIN_SET) : \
                                HAL_GPIO_WritePin(ota_esp8266_RST_GPIO_PORT, ota_esp8266_RST_GPIO_PIN, GPIO_PIN_RESET); \
                            }while(0)


/* ================= OTA配置宏(用户修改区) ================= */
#define OTA_WIFI_SSID     "your_ssid"                      /* WiFi名称，自行修改 */
#define OTA_WIFI_PWD      "your_password"                  /* WiFi密码，自行修改 */
#define OTA_SRV_DOMAIN    "000.000.00.000"                 /* 电脑局域网IP(XNET监听地址), 改成电脑实际IP */
#define OTA_SRV_PORT      "000"                           /* TCP端口 */
#define OTA_MAX_APP_SIZE  (960*1024)                       /* APP区容量上限(960KB): 收到的文件超过此大小判定失败, 固定值不用改 */
#define OTA_AUTO_START    0                                /* 0=按KEY0触发升级(防升级死循环, 测试用); 1=上电自动升级(产品模式) */
#define OTA_VERSION_STRING "VER-2.0.0-2026/8/22-16:00"     /* 固件版本号(与main.c启动横幅2.0.0一致), 格式与Bootloader解析一致 */

typedef enum
{
    OTA_ESP8266_EOK      = 0,   /* 没有错误 */
    OTA_ESP8266_ERROR    = 1,   /* 通用错误 */
    OTA_ESP8266_ETIMEOUT = 2,   /* 超时错误 */
    OTA_ESP8266_EINVAL   = 3,   /* 参数错误 */
} ota_esp8266_ret_t;



/* OTA升级标志结构体: Bootloader工程(OTA/APP/OTA/ota.h), 两工程必须同步 */
typedef struct
{
    uint32_t  OTA_up_Flag;
    uint32_t  Firelen[11];      /* 长度数组, Firelen[0]=固件长度 */
    uint8_t   OTA_Version[32];  /* OTA版本号, 格式: VER-1.0.0-2026/8/20-12:00 */
} OTA_FlagTypeDef;

#define OTA_FLAG (0xAABBAABB)  /* 升级标志值: 与我的Bootloader工程一致 */

/* 函数声明 */
uint8_t ota_esp8266_init(void);

void ota_esp8266_hw_reset(void);

uint8_t ota_esp8266_send_at_cmd(char *cmd, char *ack, uint32_t timeout);

uint8_t ota_esp8266_at_test(void);

uint8_t ota_esp8266_set_mode(uint8_t mode);

uint8_t ota_esp8266_join_ap(char *ssid, char *pwd);

uint8_t ota_esp8266_get_ip(char *buf);

uint8_t ota_esp8266_connect_tcp_server(char *server_ip, char *server_port);

uint8_t ota_esp8266_enter_unvarnished(void);

void ota_esp8266_exit_unvarnished(void);

void ota_esp8266_ensure_at_mode(void);  /* 确保模块回到AT指令模式(退出透传+关TCP), 失败回待机后调用 */

#endif // OTA_WIFI_H
