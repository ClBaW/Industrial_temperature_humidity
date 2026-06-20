#ifndef MODBUS_H
#define MODBUS_H
#include "stdint.h"
#include "stm32f4xx.h"

#include "stdio.h"
#include "string.h"

#include "./SYSTEM/usart/usart.h"
#include "./SYSTEM/delay/delay.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "rs485.h"



typedef struct
{
    uint8_t  slave_addr;         /* 从机地址 */
    uint8_t  command_code;       /* 功能码 */
    uint16_t reg_start;          /* 起始寄存器 */
    uint16_t Register_count;     /* 寄存器个数 */
    uint16_t err_cnt;            /* 连续失败次数 */
    float    temp;               /* 温度 */
    float    humi;               /* 湿度 */
}Modbus_t;

/* 温湿度数据(给LVGL用) */
typedef struct
{
    float    temp;        /* 温度 0.1C */
    float    humi;        /* 湿度 0.1RH */
}Modbus_TH_t;


extern Modbus_t dev_list[];
extern uint8_t dev_cnt;
extern QueueHandle_t lvgl_data_queue;  

void Modbus_Init(void);

uint8_t Modbus_communication_Data(uint8_t addr,uint8_t func,uint16_t reg,uint16_t regcnt,uint16_t *buf);

void Modbus_Receive_all_data(void);

void Modbus_Updata_to_LVGL(void);

uint16_t Modbus_CRC16(uint8_t *data,uint16_t len);





#endif
