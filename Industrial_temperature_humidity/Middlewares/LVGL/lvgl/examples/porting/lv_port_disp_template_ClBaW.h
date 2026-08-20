#ifndef LV_PORT_DISP_TEMPLATE_CLBAW_H
#define LV_PORT_DISP_TEMPLATE_CLBAW_H


#include "lvgl.h"
#include "BSP/LCD/lcd.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>

/* ÏÔÊ¾Æ÷·Ö±æÂÊ(240x320) */

#define BYTE_PER_PIXEL (LV_COLOR_FORMAT_GET_SIZE(LV_COLOR_FORMAT_RGB565))

void lv_port_disp_init(void);

#endif /* LV_PORT_DISP_TEMPLATE_CLBAW_H */
