#ifndef OTA_UASRT_H
#define OTA_UASRT_H

#include "stdio.h"
#include "./SYSTEM/sys/sys.h"

#define OTA_UASRT1_RX_BUF_SIZE 2048 //串口接收缓冲区大小
#define OTA_UASRT1_RX_MAX 256 //最大接收长度
#define NUM 10 //接收缓冲区个数

#define OTA_USART_UX USART1

/* 引脚定义 */
#define OTA_UART_TX_GPIO_PORT           GPIOA
#define OTA_UART_TX_GPIO_PIN            GPIO_PIN_9
#define OTA_UART_TX_GPIO_AF             GPIO_AF7_USART1
#define OTA_UART_TX_GPIO_CLK_ENABLE()   do{ __HAL_RCC_GPIOA_CLK_ENABLE(); }while(0)     /* PA时钟使能 */

#define OTA_UART_RX_GPIO_PORT           GPIOA
#define OTA_UART_RX_GPIO_PIN            GPIO_PIN_10
#define OTA_UART_RX_GPIO_AF             GPIO_AF7_USART1
#define OTA_UART_RX_GPIO_CLK_ENABLE()   do{ __HAL_RCC_GPIOA_CLK_ENABLE(); }while(0)     /* PA时钟使能 */

#define OTA_UART_INTERFACE              USART1
#define OTA_UART_IRQn                   USART1_IRQn
#define OTA_UART_IRQHandler             USART1_IRQHandler
#define OTA_UART_CLK_ENABLE()           do{ __HAL_RCC_USART1_CLK_ENABLE(); }while(0)    /* USART1时钟使能 */



typedef struct
{
    uint8_t *start;
    uint8_t *end;
} ota_uasrt_rx_ptr;

typedef struct
{
    uint16_t          URxCount;         //接收计数
    ota_uasrt_rx_ptr  URxDataPtr[NUM];  //数据个数
    ota_uasrt_rx_ptr  *URxData_IN;
    ota_uasrt_rx_ptr  *URxData_OUT;
    ota_uasrt_rx_ptr  *URxData_END;
} ota_uasrt_CB;

extern uint8_t ota_uasrt1_rx_buf[OTA_UASRT1_RX_BUF_SIZE]; /* 接收缓冲区 */
extern ota_uasrt_CB U1CB;                                 /* 串口接收控制块 */
extern UART_HandleTypeDef g_uart1_handle;  /* USART1句柄(定义在ota_uasrt.c) */

void ota_uasrt_init(uint32_t baudrate);
void ota_uasrt_Rx_ptr_init(void);
void ota_usart_test(void);

#endif // OTA_UASRT_H
