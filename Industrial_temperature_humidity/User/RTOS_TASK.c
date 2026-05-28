#include "RTOS_TASK.h"

/*FreeRTOS任务*/

/* START_TASK 任务 定义
 * 包括: 任务优先级 栈大小 任务句柄
 */
#define START_TASK_PRIO     1           /* 任务优先级 */
#define START_STK_SIZE      512         /* 任务栈大小 后面自己输出了；所以大就大*/
TaskHandle_t StartTask_Handler;         /* 任务句柄 */
void start_task(void *pvParameters);    /* 任务函数 */

/* LV_DEMO_TASK 任务 定义
 * 包括: 任务优先级 栈大小 任务句柄
 */
#define LV_DEMO_TASK_PRIO   3           /* 任务优先级 */
#define LV_DEMO_STK_SIZE    2048        /* 任务栈大小 */
TaskHandle_t LV_DEMOTask_Handler;       /* 任务句柄 */
void lv_demo_task(void *pvParameters);  /* 任务函数 */


/* MODBUS_TASK 任务 定义
 * 包括: 任务优先级 栈大小 任务句柄
 */
#define MODBUS_TASK_PRIO    2           /* 任务优先级 */
#define MODBUS_STK_SIZE     512         /* 任务栈大小 */
TaskHandle_t ModbusTask_Handler;        /* 任务句柄 */
void modbus_task(void *pvParameters);   /* 任务函数 */

/* OTA_WIFI_TASK 任务 定义
 * 包括: 任务优先级 栈大小 任务句柄
 */
#define OTA_WIFI_TASK_PRIO  4           /* 高于LVGL(3): 避免刷屏抢CPU导致UART溢出丢字节 */           /* 任务优先级 */
#define OTA_WIFI_STK_SIZE   512         /* 任务栈大小 */
TaskHandle_t OTAWifiTask_Handler;       /* 任务句柄 */
void ota_wifi_task(void *pvParameters); /* 任务函数 */


// /* LED_TASK 任务 定义
//  * 包括: 任务优先级 栈大小 任务句柄
//  */
// #define LED_TASK_PRIO       4           /* 任务优先级 */
// #define LED_STK_SIZE        128         /* 任务栈大小 */
// TaskHandle_t LEDTask_Handler;           /* 任务句柄 */
// void led_task(void *pvParameters);      /* 任务函数 */

// /**
//  * @description:  led_task用于测试rtos调度是否进行
//  * @param {void} *pvParameters
//  * @return {*}
//  */
// void led_task(void *pvParameters)
// {
//     pvParameters = pvParameters;

//     while(1)
//     {
//         //LED1_TOGGLE();
//         vTaskDelay(1000);
//     }
// }





/**
 * @description: 打印rtos内存信息
 * @param {*}
 * @return {*}
 */
#if RTOS_LOOK_MEM
void rtos_mem_report(void)
{
    UBaseType_t n, got, i;
    TaskStatus_t *sts;

    printf("\r\n===== RTOS 内存报告 =====\r\n");

    printf("堆: 总量=%u (字节)当前空闲=%u (字节)历史最低=%u(字节)\r\n",
           (uint32_t)configTOTAL_HEAP_SIZE ,
           (uint32_t)xPortGetFreeHeapSize(),
           (uint32_t)xPortGetMinimumEverFreeHeapSize());

    printf("[1] 任务数=%u\r\n", (uint32_t)uxTaskGetNumberOfTasks());

    n = uxTaskGetNumberOfTasks();
    //申请内存
    sts = (TaskStatus_t *)pvPortMalloc(n * sizeof(TaskStatus_t));
    printf("[2] 分配成功, 地址=%08X\r\n", (uint32_t)(uintptr_t)sts);
    if(sts == NULL)
    {
        printf("任务表: 申请失败 n=%u\r\n", (uint32_t)n);
    }
    else
    {
        got = uxTaskGetSystemState(sts, n, NULL);
        printf("[3] 任务数=%u\r\n", (uint32_t)got);
        for(i = 0; i < got; i++)
        {
            printf("[4-%u] %-16s 优先级=%u 栈最小剩余=%u 字 (可以直接和任务栈对比)\r\n", (uint32_t)(i+1),
                   sts[i].pcTaskName,
                   (uint32_t)sts[i].uxCurrentPriority,
                   (uint32_t)(sts[i].usStackHighWaterMark));
        }
        vPortFree(sts);

    }
}
#endif


/**
 * @description: 看lvgl内存信息
 * 碎片尽量比例< 10%	
 * 最大快  20KB左右 当然越大越好
 * 空闲 	> 40%
 * @return {*}
 */
#if LVGL_LOOK_MEM
void lvgl_mem_report(void)
{
    lv_mem_monitor_t mon;
    lv_mem_monitor(&mon);

    printf("LVGL内存: 总量=%u 已用=%u 峰值=%u 空闲=%u 最大块=%u 碎片=%u%%\r\n",
           (uint32_t)mon.total_size,
           (uint32_t)(mon.total_size - mon.free_size),
           (uint32_t)mon.max_used,
           (uint32_t)mon.free_size,
           (uint32_t)mon.free_biggest_size,
           (uint32_t)mon.frag_pct);
}
#endif

/**
 * @description: 查看哪个栈溢出
 * #define configCHECK_FOR_STACK_OVERFLOW                  2  
 * 使用前把这个设置为2； 然后如果要注释掉的话，必须关掉了 即设置为0
 * @param {TaskHandle_t} xTask
 * @param {char} *pcTaskName
 * @return {*}
 */
// void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
// {
//     (void)xTask;
//     printf("[STACK-OVERFLOW] task=%s\r\n", pcTaskName);
//     while(1)
//     {
//     }
// }


/**
 * @brief       lvgl_demo入口函数
 * @param       无
 * @retval      无
 */
void RTOS_TASK(void)
{

    lv_init();                                          /* lvgl系统初始化 */
    lv_port_disp_init();                                /* lvgl显示接口初始化,必须放在lv_init()的后面 */
    lv_port_indev_init();                               /* lvgl输入接口初始化,必须放在lv_init()的后面 */

    xTaskCreate((TaskFunction_t )start_task,            /* 任务函数 */
                (const char*    )"start_task",          /* 任务名称 */
                (uint16_t       )START_STK_SIZE,        /* 任务栈大小 */
                (void*          )NULL,                  /* 传入给任务函数的参数 */
                (UBaseType_t    )START_TASK_PRIO,       /* 任务优先级 */
                (TaskHandle_t*  )&StartTask_Handler);   /* 任务句柄 */

    vTaskStartScheduler();                              /* 开启任务调度 */
}

/**
 * @brief       start_task
 * @param       pvParameters : 传入参数(未用到)
 * @retval      无
 */
void start_task(void *pvParameters)
{
    pvParameters = pvParameters;

    taskENTER_CRITICAL();           /* 进入临界区 */

    /* 创建LVGL任务 */
    xTaskCreate((TaskFunction_t )lv_demo_task,
                (const char*    )"lv_demo_task",
                (uint16_t       )LV_DEMO_STK_SIZE,
                (void*          )NULL,
                (UBaseType_t    )LV_DEMO_TASK_PRIO,
                (TaskHandle_t*  )&LV_DEMOTask_Handler);



    /* Modbus采集任务创建 */
    if(xTaskCreate((TaskFunction_t )modbus_task,
                   (const char*    )"modbus_task",
                   (uint16_t       )MODBUS_STK_SIZE,
                   (void*          )NULL,
                   (UBaseType_t    )MODBUS_TASK_PRIO,
                   (TaskHandle_t*  )&ModbusTask_Handler) != pdPASS)
    {
        printf("[MODBUS] xTaskCreate fail!\r\n");
    }

    /* OTA_WIFI任务创建 */
    if(xTaskCreate((TaskFunction_t )ota_wifi_task,
                   (const char*    )"ota_wifi_task",
                   (uint16_t       )OTA_WIFI_STK_SIZE,
                   (void*          )NULL,
                   (UBaseType_t    )OTA_WIFI_TASK_PRIO,
                   (TaskHandle_t*  )&OTAWifiTask_Handler) != pdPASS)
    {
        printf("[OTA_WIFI] xTaskCreate fail!\r\n");
    }

        // /* LED任务创建 */
    // xTaskCreate((TaskFunction_t )led_task,
    //             (const char*    )"led_task",
    //             (uint16_t       )LED_STK_SIZE,
    //             (void*          )NULL,
    //             (UBaseType_t    )LED_TASK_PRIO,
    //             (TaskHandle_t*  )&LEDTask_Handler);

    taskEXIT_CRITICAL();            /* 退出临界区 */

    #if RTOS_LOOK_MEM

    vTaskDelay(5000);           /* 等 modbus/ota/lvgl 各任务先跑 5 秒, 栈高水位才是真峰值 */
    rtos_mem_report();

    #endif

    vTaskDelete(StartTask_Handler); /* 删除开始任务 */
}

/**
 * @brief       LVGL任务
 * @param       pvParameters : 传入参数(未用到)
 * @retval      无
 */
void lv_demo_task(void *pvParameters)
{
    pvParameters = pvParameters;

    ui_init();         /* 测试demo */

    // lv_obj_invalidate(lv_screen_active());       /* 强制首帧重绘 */

    while(1)
    {
        lv_timer_handler(); /* LVGL定时器 */
        vTaskDelay(5);
    }
}

/**
 * @description:  OTA_WIFI任务: 轮询OTA状态机(KEY0触发升级)
 * @param {void} *pvParameters
 * @return {*}
 */
void ota_wifi_task(void *pvParameters)
{
    pvParameters = pvParameters;

    while(1)
    {
        ota_Statemachine_while();   /* 7态状态机轮询 */
        vTaskDelay(10);             /* 10ms 轮询周期 */
    }
}





/**
 * @description: modbus任务和一些调试块
 * @param {void} *pvParameters
 * @return {*}
 */
void modbus_task(void *pvParameters)
{
     pvParameters = pvParameters;
    while(1)
    {
        Modbus_Receive_all_data();
        Modbus_Updata_to_LVGL();

        #if RTOS_LOOK_MEM
            /* ---------- FreeRTOS堆泄漏监视: 每60秒打一次 防止内存泄漏了 ---------- */
            {
                static uint32_t tm_last = 0;
                if(xTaskGetTickCount() - tm_last >= 60000)
                {
                    tm_last = xTaskGetTickCount();
                    printf("[堆] 当前空闲=%u 历史最低=%u\r\n",
                        (uint32_t)xPortGetFreeHeapSize(),
                        (uint32_t)xPortGetMinimumEverFreeHeapSize());      
                }
            }
        #endif

        #if LVGL_LOOK_MEM
        /* ---------- LVGL内存监视: 每60秒单独打一次 ---------- */
            {
                static uint32_t lv_tm_last = 0;
                if(xTaskGetTickCount() - lv_tm_last >= 60000)
                {
                    lv_tm_last = xTaskGetTickCount();
                    lvgl_mem_report();
                }
            }
        #endif

        #if MODBUS_DEBUG
            /* ---------- 无仪表诊断(仅在连续读失败时触发一次) ---------- */
            {
                static uint8_t fail_cnt = 0;
                static uint8_t diag_done = 0;
                if(dev_list[0].temp <= 0.0f && dev_list[0].humi <= 0.0f)
                {
                    fail_cnt++;
                    if(fail_cnt == 2 && !diag_done)   /* 约3秒无数据: 软万用表-采PA3(连TP8485的RO)电平 */
                    {
                        GPIO_InitTypeDef gpio_diag;
                        gpio_diag.Pin = GPIO_PIN_3;
                        gpio_diag.Mode = GPIO_MODE_INPUT;
                        gpio_diag.Pull = GPIO_NOPULL;
                        gpio_diag.Speed = GPIO_SPEED_FREQ_HIGH;
                        HAL_GPIO_Init(GPIOA, &gpio_diag);
                        uint8_t diag_hi = 0, diag_lo = 0;
                        for(uint8_t di = 0; di < 8; di++)
                        {
                            if(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_3)) diag_hi++; else diag_lo++;
                            vTaskDelay(pdMS_TO_TICKS(2));
                        }
                        printf("[MODBUS-DIAG] PA3(RO) hi=%d lo=%d -> %s\r\n", diag_hi, diag_lo,
                            diag_hi > diag_lo ? "RO全是1: TP8485在驱动(芯片OK-近端通)"
                                                : "RO多为0/悬浮: 芯片无电或PA3没接到RO");
                        /* 恢复USART2_RX(PA3)的AF7模式 */
                        gpio_diag.Mode = GPIO_MODE_AF_PP;
                        gpio_diag.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
                        gpio_diag.Alternate = GPIO_AF7_USART2;
                        HAL_GPIO_Init(GPIOA, &gpio_diag);
                    }
                    if(fail_cnt == 3 && !diag_done)   /* 约4秒无数据: 发FF04重启帧验证链路回显 */
                    {
                        uint8_t ff04[8] = {0x01, 0x06, 0xFF, 0x04, 0x00, 0x00, 0xF8, 0x1F}; /* 重启设备 */
                        uint8_t rx_diag[RS485_REC_LEN + 2] = {0};
                        uint8_t rl_diag = 0;
                        RS485_RE(1);
                        HAL_UART_Transmit(&rs458_handler, ff04, 8, 1000);
                        vTaskDelay(pdMS_TO_TICKS(1));
                        RS485_RE(0);
                        for(uint8_t di = 0; di < 10; di++)
                        {
                            rs485_receive_data(rx_diag, &rl_diag);
                            if(rl_diag) break;
                            vTaskDelay(pdMS_TO_TICKS(5));
                        }
                        diag_done = 1;
                        printf("[MODBUS-DIAG] FF04 reboot probe: reply rx=%d ", rl_diag);
                        if(rl_diag >= 8)
                        {
                            printf("[%02X %02X %02X %02X %02X %02X %02X %02X] ",  rx_diag[0], rx_diag[1], rx_diag[2], rx_diag[3], rx_diag[4], rx_diag[5], rx_diag[6], rx_diag[7]);
                            if(rx_diag[0] == 0x01 && rx_diag[1] == 0x06 && rx_diag[2] == 0xFF && rx_diag[3] == 0x04)
                                printf("-> 模块在线回显! 全链路通\r\n");
                            else
                                printf("-> 有回包但内容不对\r\n");
                        }
                        else
                        {
                            printf("-> 无回包: 近端/模块端问题-可用USB转485交叉验证\r\n");
                        }
                    }
                }
                else
                {
                    fail_cnt = 0;   /* 已读到数据: 恢复计数 */
                }
            }
        #endif

        vTaskDelay(1000);   /* 1秒轮询周期 */
    }
}



