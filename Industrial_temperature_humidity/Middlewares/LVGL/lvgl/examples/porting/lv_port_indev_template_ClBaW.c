/**
 * @file        lv_port_disp_template.c lv_port_idev_template.c
 * @brief       自己通过正点原子他的BSP 以及 官方lv_port_disp_template.c lv_port_idev_template.c进行修改 移植其他的话可以参考
 *              移植的呢 重要的就是输出和输入这两个
 *              如果要看原版再文件目录也有就是lv_port_disp_template.c lv_port_idev_template.c
 */ 
#include "lv_port_indev_template_ClBaW.h"

static void touchpad_read_cb(lv_indev_t * indev, lv_indev_data_t * data);

/**
 * @description: 初始化输入设备(触摸模块)
 * @return {*}
 */
void lv_port_indev_init(void)
{   
    /* 初始化触摸屏(内部会识别触摸IC并初始化) */
    tp_dev.init();
    
    /* 创建输入设备 */
    lv_indev_t * indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, touchpad_read_cb);
}

/**
 * @description: 触摸读取回调
 * @param {lv_indev_t *} indev 输入设备
 * @param {lv_indev_data_t *} data 触摸数据
 * @return {*}
 */
static void touchpad_read_cb(lv_indev_t * indev, lv_indev_data_t * data)
{
    tp_dev.scan(0);                         /* 扫描触摸屏 */

    if (tp_dev.sta & TP_PRES_DOWN)          /* 按下 */
    {
        data->point.x = tp_dev.x[0];
        data->point.y = tp_dev.y[0];
        data->state = LV_INDEV_STATE_PRESSED;
    }
    else                                    /* 松开 */
    {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}
