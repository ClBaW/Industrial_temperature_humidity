#ifndef __LVGL_DEMO_H
#define __LVGL_DEMO_H

#include "./BSP/LED/led.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>

#include "lvgl.h"
#include "lv_port_disp_template_ClBaW.h"
#include "lv_port_indev_template_ClBaW.h"

#include "ui.h"
#include "modbus.h"
#include "ota_Statemachine.h"


/* ====================================调试开关==================================== */

#define RTOS_LOOK_MEM  0       //看rtos内存信息
#define LVGL_LOOK_MEM  0       //看lvgl内存信息
#define MODBUS_DEBUG   0       //modbus调试开关

void RTOS_TASK(void);

#endif
