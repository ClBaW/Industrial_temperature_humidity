#ifndef _RS485_H
#define _RS485_H

#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/delay/delay.h"
#include "FreeRTOS.h"
#include "task.h"


/* 串口 引脚 定义 */
#define RS485_UX                        USART2
#define RS485_UX_CLK_ENABLE()           do{ __HAL_RCC_USART2_CLK_ENABLE(); }while(0)   /* USART2 时钟使能 */
#define RS485_UX_IRQn                   USART2_IRQn
#define RS485_UX_IRQHandler             USART2_IRQHandler

#define RS485_TX_GPIO_PORT              GPIOA
#define RS485_TX_GPIO_PIN               GPIO_PIN_2
#define RS485_TX_GPIO_AF                GPIO_AF7_USART2
#define RS485_TX_GPIO_CLK_ENABLE()      do{ __HAL_RCC_GPIOA_CLK_ENABLE(); }while(0)   /* TX 引脚时钟使能 */

#define RS485_RX_GPIO_PORT              GPIOA
#define RS485_RX_GPIO_PIN               GPIO_PIN_3
#define RS485_RX_GPIO_AF                GPIO_AF7_USART2
#define RS485_RX_GPIO_CLK_ENABLE()      do{ __HAL_RCC_GPIOA_CLK_ENABLE(); }while(0)   /* RX 引脚时钟使能 */

/* 方向控制引脚RS485_RE: PG8; 最关键配置；注意这个配置；控制RS485发送和接收的方向的*/
#define RS485_RE_GPIO_PORT              GPIOG
#define RS485_RE_GPIO_PIN               GPIO_PIN_8
#define RS485_RE_GPIO_CLK_ENABLE()      do{ __HAL_RCC_GPIOG_CLK_ENABLE(); }while(0)   /* 方向引脚时钟使能 (GPIOG) */


/* RS485 发送/接收方向控制的宏定义函数 */
#define RS485_RE(x)                     HAL_GPIO_WritePin(RS485_RE_GPIO_PORT, RS485_RE_GPIO_PIN, x ? GPIO_PIN_SET : GPIO_PIN_RESET) /* x=1 发送 x=0 接收 */


#define RS485_REC_LEN                   64                       /* 最大接收长度 */

extern UART_HandleTypeDef rs458_handler;  

void rs485_init(uint32_t baudrate);                          
void rs485_receive_data(uint8_t *buf, uint8_t *len);          

#endif
