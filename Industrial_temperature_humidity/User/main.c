/*
 * @Author: ClBaW
 * @Date: 2026-05-28 10:00:00
 * @LastEditTime: 2026-09-08 23:46:14
 */
#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/usart/usart.h"
#include "./SYSTEM/delay/delay.h"
#include "./BSP/LED/led.h"
#include "./BSP/KEY/key.h"
#include "./BSP/SRAM/sram.h"
#include "RTOS_TASK.h"



/* 内存：分配 内部 SRAM 128KB
内部的话 
    1. #define LV_MEM_SIZE (64 * 1024U)                         lvgl的内存我设置64K
    2. #define configTOTAL_HEAP_SIZE ((size_t)(24 * 1024))      freertos 我设置24K 稳态已用 14KB / 峰值 17KB/ 空闲 9KB
    3.其他 14KB
内部SRAM合计: 128KB, 已用 100多KB, 剩余 20多KB
外部SRAM外部 SRAM 1MB
    显示缓冲区:存储块3(FSMC_NE3)地址范围: 0X6800 0000 ~ 0X6BFF FFFF
    做了俩个版本
    1.30 行单缓冲 19kb
    2.由于外部大所以做了 全屏双缓冲    全屏双缓冲(FULL 300KB x 2 = 600KB) 
*/

int main(void)
{
    __enable_irq();                     /* 恢复全局中断: Bootloader跳转前关中断(PRIMASK=1), APP必须自行恢复, 否则SysTick/LVGL全卡死 */
    HAL_Init();                         /* 初始化HAL库 */
    sys_stm32_clock_init(336, 8, 2, 7); /* 设置时钟,168Mhz */
    delay_init(168);                    /* 延时初始化 */
    usart_init(115200);                 /* 串口初始化为115200 */
    led_init();                         /* 初始化LED */
    beep_init();                        /*初始化蜂鸣器*/
    key_init();                         /* 初始化按键 */
    sram_init();                        /* SRAM初始化 */
    

    ota_Statemachine_init();            /* ota全部模块初始化；包括：24c02初始化，25q128初始化，esp8266初始化，开启中断 */
    Modbus_Init();                      /* modbus初始化 */
    
    RTOS_TASK();                        /* 运行FreeRTOS任务 */
}


