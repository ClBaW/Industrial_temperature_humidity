#ifndef OTA_WIFI_USART_H
#define OTA_WIFI_USART_H

#include "stdio.h"
#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/usart/usart.h" 
#include <stdarg.h>
#include <stdio.h>
#include <string.h>


/* 串口接收参数(环形缓冲区大小) */
#define OTA_ESP8266_UART_RX_BUF_SIZE 2048   /* 环形接收缓冲区大小 */

/* 串口定义 */
#define OTA_ESP8266_UART_INTERFACE  USART3
#define OTA_ESP8266_UART_IRQn       USART3_IRQn
#define OTA_ESP8266_UART_CLK_ENABLE()    do{ __HAL_RCC_USART3_CLK_ENABLE(); }while(0)  /* USART3时钟使能 */

/* 引脚定义 */
#define OTA_ESP8266_UART_TX_GPIO_PORT           GPIOB
#define OTA_ESP8266_UART_TX_GPIO_PIN            GPIO_PIN_10
#define OTA_ESP8266_UART_TX_GPIO_AF             GPIO_AF7_USART3
#define OTA_ESP8266_UART_TX_GPIO_CLK_ENABLE()   do{ __HAL_RCC_GPIOB_CLK_ENABLE(); }while(0)  /* PB时钟使能 */

#define OTA_ESP8266_UART_RX_GPIO_PORT           GPIOB
#define OTA_ESP8266_UART_RX_GPIO_PIN            GPIO_PIN_11
#define OTA_ESP8266_UART_RX_GPIO_AF             GPIO_AF7_USART3
#define OTA_ESP8266_UART_RX_GPIO_CLK_ENABLE()   do{ __HAL_RCC_GPIOB_CLK_ENABLE(); }while(0)  /* PB时钟使能 */



/* 接收控制块: 环形计数器方案(ISR写URxWriteCnt, 主循环读URxReadCnt)
    ！！好处：不需要临界区保护；并且读写计数器的操作是原子操作，不存在读写冲突
*/
typedef struct
{
    volatile uint16_t  URxWriteCnt;  /* 写计数 */
    uint16_t           URxReadCnt;   /* 读计数 */
} ota_esp8266_uart_CB;

extern uint8_t ota_esp8266_uart_rx_buf[OTA_ESP8266_UART_RX_BUF_SIZE];   /* 环形接收缓冲区 */
extern ota_esp8266_uart_CB U3CB;                                        /* 串口接收控制块 */
extern UART_HandleTypeDef g_esp8266_uart_handle;                        /* 串口句柄(定义在ota_wifi_usart.c) */

void ota_esp8266_uart_init(uint32_t baudrate);

uint16_t ota_esp8266_uart_stream_read(uint8_t *buf, uint16_t maxlen);

void ota_esp8266_uart_printf(const char *fmt, ...);


#endif // OTA_WIFI_USART_H
