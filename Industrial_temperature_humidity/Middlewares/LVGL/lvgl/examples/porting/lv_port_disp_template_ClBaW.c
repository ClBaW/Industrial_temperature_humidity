/**
 * @file        lv_port_disp_template.c lv_port_idev_template.c
 * @brief       自己通过正点原子他的BSP 以及 官方lv_port_disp_template.c lv_port_idev_template.c进行修改 移植其他的话可以参考
 *              移植的呢 重要的就是输出和输入这两个
 *              如果要看原版再文件目录也有就是lv_port_disp_template.c lv_port_idev_template.c
 */ 
#include "lv_port_disp_template_ClBaW.h"


static void disp_flush_cb(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map);

#define DISP_BUF_LINES 30            /* PARTIAL模式的缓冲行数 */
#define DISP_W   480                /* 逻辑宽(横屏, 与 lcd_display_dir(1) 下的 lcddev.width 一致) */
#define DISP_H   320                /* 逻辑高 */
#define DISP_FULL_DOUBLE 1          /* 0=运行字节数=30行单缓冲(PARTIAL 19.2KB); 1=全屏双缓冲(FULL 300KB x 2 = 600KB) */

#if DISP_FULL_DOUBLE
/* 全屏双缓冲: 2x320x480x2=600KB, 外部SRAM 0x68004B00起(接在旧 30 行缓冲段之后) */
static uint8_t s_disp_buf1[DISP_W * DISP_H * 2] __attribute__((at(0x68004B00)));
static uint8_t s_disp_buf2[DISP_W * DISP_H * 2] __attribute__((at(0x68004B00 + DISP_W * DISP_H * 2)));
#else
/* 30行单缓冲: 19.2KB @ 0x68000000 */
static uint8_t s_disp_buf[DISP_W * DISP_BUF_LINES * 2] __attribute__((at(0x68000000)));
#endif

void lv_port_disp_init(void)
{
    uint8_t *buf_1;   //内存缓冲区1
    uint8_t *buf_2;   //内存缓冲区2
    uint32_t buf_size;//缓冲区大小
    lv_display_render_mode_t render_mode;

    /* 1.初始化LCD */
    lcd_init();                     /* 初始化LCD(FSMC 16bit), 确认 lcddev.width/height */
    lcd_display_dir(1);             /* 纵屏 */

    /* 关键! LVGL的“时钟源”: 使用 FreeRTOS 系统节拍(1ms, configTICK_RATE_HZ=1000) */
    lv_tick_set_cb((lv_tick_get_cb_t)xTaskGetTickCount);

#if DISP_FULL_DOUBLE
    buf_1 = s_disp_buf1;
    buf_2 = s_disp_buf2;
    buf_size = DISP_W * DISP_H * 2;
    render_mode = LV_DISPLAY_RENDER_MODE_FULL;
#else
    buf_1 = s_disp_buf;
    buf_2 = NULL;                  /* 单缓冲 */
    buf_size = DISP_W * DISP_BUF_LINES * 2;
    render_mode = LV_DISPLAY_RENDER_MODE_PARTIAL;
#endif

    /* 2.初始化LVGL显示设备 */
    lv_display_t * disp = lv_display_create(DISP_W, DISP_H);
    lv_display_set_flush_cb(disp, disp_flush_cb);
    lv_display_set_buffers(disp, buf_1, buf_2, buf_size, render_mode);

    /* 3.颜色格式: RGB565 */
    lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);

    // printf("显示缓冲: %u 字节 @0x%08X\r\n",
    //        (uint32_t)buf_size,
    //        (uint32_t)(uintptr_t)buf_1);
}


/**
 * @description: 显示刷新回调, LVGL渲染完成后调用, 把缓冲写入LCD
 * @param {lv_display_t *} disp 显示设备
 * @param {lv_area_t *} area 需要刷新的区域
 * @param {uint8_t *} px_map 缓冲数据(RGB565)
 * @return {*}
 */
static void disp_flush_cb(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map)
{
    lcd_color_fill(area->x1, area->y1, area->x2, area->y2, (uint16_t*)px_map);

    lv_display_flush_ready(disp);               /* 通知LVGL刷新完成 */
}
