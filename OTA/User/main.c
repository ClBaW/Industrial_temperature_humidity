#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/usart/usart.h"
#include "./SYSTEM/delay/delay.h"
#include "./BSP/LED/led.h"
#include "./BSP/KEY/key.h"
#include "./BSP/TIMER/btim.h"
#include "ota.h"



int main(void)
{   
    HAL_Init();                           /* 初始化HAL库 */
    sys_stm32_clock_init(336, 8, 2, 7);   /* 设置时钟,168Mhz */
    delay_init(168);                      /* 延时初始化 */
    led_init();                           /* 初始化LED */
    key_init();                           /* 初始化按键 */
    btim_timx_int_init(10 - 1, 8400 - 1); /* 初始化定时器 */


    ota_init();                           /* 初始化OTA */
    ota_main();                           /* ota主函数 */
    

    while (1)
    {
        ota_while();

    }
}
