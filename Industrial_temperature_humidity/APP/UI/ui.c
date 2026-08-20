#include "ui.h"

/* ========== 组件的控制句柄 ========== */
static lv_obj_t *scr_home, *scr_alarm, *scr_wifi;
static lv_obj_t *st_home, *st_alarm, *lb_t, *lb_h;     /* 状态字/首页大数字 */
static lv_obj_t *lb_thr_t, *lb_thr_h;                  /* 首页报警阈值行(温度/湿度) */
static lv_obj_t *sl[4], *lbv[4];                       /* 告警页: 滑块+数值 */
static lv_obj_t *hint, *ta_pwd, *kb;                   /* 已保存/密码框/软键盘 */
static lv_obj_t *ta_ssid, *ta_srv, *lb_stat, *btn_up;  /* WiFi 名称/服务器/连接状态/升级按钮 */
static lv_obj_t *lb_prog;                              /* OTA升级进度(红色) */
static lv_obj_t *btn_eye, *btn_eye_s;                  /* 密码/服务器 显示隐藏圆按钮(灰**** 蓝明文) */

/**
 * @description: 从modbus任务通过队列获取最新温湿度数据
 * @param {float} *t
 * @param {float} *h
 * @return {*}
 */
static int data_get_th(float *t, float *h)
{
    Modbus_TH_t th;
    /* 队列空: 返回 0, 没有数据; 队列非空: 返回 1, 并把数据写入 t/h */
    if(xQueuePeek(lvgl_data_queue, &th, 0) != pdTRUE)
    {
        return 0;
    }
    *t = th.temp;  *h = th.humi;
    return 1;
}

/* --- OTA 状态机 → UI 状态: 0空闲 1连接中 2已连接/升级中 3失败 --- */
static int data_ota_stat(void)
{
    switch(ota_state) {
        case OTA_STATE_WIFI_CONNECT: return 1;
        case OTA_STATE_TCP_CONNECT:
        case OTA_STATE_PASSTHROUGH:
        case OTA_STATE_RECEIVE:
        case OTA_STATE_FINISH:       return 2;
        case OTA_STATE_ERROR:        return 3;
        default:                     return 0;
    }
}

/**
 * @description: 创建标签以及设置字体
 * @param {lv_obj_t} *p 父对象
 * @param {char} *t 文本内容
 * @param {lv_font_t} *f 字体
 * @return {*}
 */
static lv_obj_t *lbl(lv_obj_t *p, const char *t, const lv_font_t *f)
{
    // 创建标签
    lv_obj_t *l = lv_label_create(p);
    // 设置标签文本
    lv_label_set_text(l, t);
    // 设置标签字体
    lv_obj_set_style_text_font(l, f, 0);
    return l;
}

/**
 * @description: 创建带颜色的标签(基于 lbl 再设置文字颜色)
 * @param {lv_obj_t} *p 父对象
 * @param {char} *t 文本内容
 * @param {lv_font_t} *f 字体
 * @param {lv_color_t} c 文字颜色
 * @return {*}
 */

static lv_obj_t *lbl_color(lv_obj_t *p, lv_color_t c)
{
    // 直接给标签对象设置文字颜色
    lv_obj_set_style_text_color(p, c, 0);
    return p;
}

/*=============================Shou_ye=============================*/

/**
 * @description:  首页: 温湿度大数字
 * @return {*}
 */
static void Shou_ye(void)
{
    scr_home = lv_obj_create(NULL);
    lv_obj_set_pos(lbl(scr_home, "温湿度监测", &cn_font16), 16, 12);
    st_home = lbl(scr_home, "正常", &cn_font16);
    lbl_color(st_home, C_GREEN);
    lv_obj_set_pos(st_home, 400, 12);

    const char *nm[2] = {"温度", "湿度"};
    const char *un[2] = {"℃", "%"};
    lv_obj_t *val[2];
    //设置整体位置 屏幕温湿度
    for(int i = 0; i < 2; i++)
    {
        lv_obj_set_pos(lbl(scr_home, nm[i], &cn_font16), 33 + i * 236, 65);
        val[i] = lbl(scr_home, "--.-", &cn_font48);
        lv_obj_set_pos(val[i], 40 + i * 236, 95);
        lv_obj_set_pos(lbl(scr_home, un[i], &cn_font16), 160 + i * 236, 140);
    }
    lb_t = val[0];
    lb_h = val[1];

    /* 底部报警阈值两行(与告警页共用 thr[], 定时器更新) */
    lb_thr_t = lbl(scr_home, "", &cn_font16);
    lv_obj_align(lb_thr_t, LV_ALIGN_BOTTOM_LEFT, 35, -120);
    lb_thr_h = lbl(scr_home, "", &cn_font16);
    lv_obj_align(lb_thr_h, LV_ALIGN_BOTTOM_LEFT, 35, -95);
}

/*=============================Alarm=============================*/
static float thr[4] = {45.0f, 10.0f, 85.0f, 20.0f};    /* 默认值 */

/**
 * @description: 显示滑块的值
 * @param {int} i
 * @return {*}
 */
static void Sliader_value(int i)
{
    /* 获取滑块的值 */
    int v = lv_slider_get_value(sl[i]);
    /* 必须区分温度湿度符号 */
    if(i < 2)
    {
        lv_label_set_text_fmt(lbv[i], "%d.%d℃", v / 10, v % 10);
    }
    else
    {
        lv_label_set_text_fmt(lbv[i], "%d.%d%%", v / 10, v % 10);
    }
}
/**
 * @description: Alarm 四个滑块的回调函数
 * @param {lv_event_t} *e
 * @return {*}
 */
static void Sliader_cb(lv_event_t *e)
{
    lv_obj_t *s = lv_event_get_target_obj(e);               //获取对象
    for(int i = 0; i < 4; i++)
    {
        if(sl[i] == s) Sliader_value(i);  //显示对应的值再滑块后面
    }
}

/**
 * @description: 设置每个滑块值并同步阈值到数值显示
 * @return {*}
 */
static void thr_sync(void)
{
    for(int i = 0; i < 4; i++)
    {
        lv_slider_set_value(sl[i], (int)(thr[i] * 10.0f + 0.5f), LV_ANIM_OFF);
        Sliader_value(i);
    }
}

/**
 * @description:lv_timer_create回调 1.5s后清提示
 * @param {lv_timer_t} *tm
 * @return {*}
 */
static void hint_off_cb(lv_timer_t *tm)
{
    lv_label_set_text(hint, "");
}

/**
 * @description: 保存阈值数据(写24c02 EEPROM, 值×10 小端)
 * @return {*}
 */
static void data_thr_save(void)
{
    for(int i = 0; i < 4; i++)
    {
        uint16_t v = (uint16_t)(thr[i] * 10.0f + 0.5f);
        ota_24C02_WriteByte(THR_ADDR + 2 * i,     v & 0xFF);
        ota_24C02_WriteByte(THR_ADDR + 2 * i + 1, v >> 8);
    }
}

/**
 * @description: 读取阈值数据(读24c02 EEPROM, 新片返回默认值)
 * @return {*}
 */
static void data_thr_load(void)
{
    uint8_t b[8];
    ota_24C02_ReadData(THR_ADDR, b, 8);
    /* 新片→默认 */
    if((b[0] & b[1] & b[2] & b[3] & b[4] & b[5] & b[6] & b[7]) == 0xFF)
    {
        return;
    }
    for(int i = 0; i < 4; i++)
    {
        thr[i] = (float)(b[2 * i] | (b[2 * i + 1] << 8)) / 10.0f;
    }
}

/**
 * @description: Alarm ：保存按钮的回调函数
 * @param {lv_event_t} *e
 * @return {*}
 */
static void save_cb(lv_event_t *e)
{
    for(int i = 0; i < 4; i++)
    {
      thr[i] = (float)lv_slider_get_value(sl[i]) / 10.0f;
    }
    data_thr_save();    //保存阈值
    lv_label_set_text(hint, "状态已保存");

    /* 定时1.5s取消已保存提示 */
    lv_timer_t *t = lv_timer_create(hint_off_cb, 1500, NULL);
    lv_timer_set_repeat_count(t, 1);         // 只触发一次
}
/**
 * @description: Alarm ： 取消按钮: 恢复原值
 * @param {lv_event_t} *e
 * @return {*}
 */
static void cancel_cb(lv_event_t *e)
{
    data_thr_load(); //取消阈值
    lv_label_set_text(hint, "状态已取消");
    thr_sync();
    /* 定时1.5s取消已保存提示 */
    lv_timer_t *t = lv_timer_create(hint_off_cb, 1500, NULL);
    lv_timer_set_repeat_count(t, 1);         // 只触发一次
}

/**
 * @description: 告警: 4滑块+保存/取消
 * @return {*}
 */
static void Alarm(void)
{
    /* 1. 设置页面名；与Shou_ye 一致即可*/
    scr_alarm = lv_obj_create(NULL);
    lv_obj_set_pos(lbl(scr_alarm, "警告设置", &cn_font16), 16, 12);
    st_alarm = lbl(scr_alarm, "正常", &cn_font16);
    lbl_color(st_alarm, C_GREEN);
    lv_obj_set_pos(st_alarm, 400, 12);

    /* 2. 设置主要组件；设置4个滑块的名字 并弄滑块来改值 */
    const char *nm[4] = {"温度上限", "温度下限", "湿度上限", "湿度下限"};
    /* 2.1一行: 名字+滑块+数值  温度上限 + 滑块 + 数值*/
    for(int i = 0; i < 4; i++)
    {
        /* 2.1.1名字 */
        lv_obj_set_pos(lbl(scr_alarm, nm[i], &cn_font16), 35, 60 + i * 36);
        /* 2.1.2创建滑块 */
        sl[i] = lv_slider_create(scr_alarm);
        lv_obj_set_pos(sl[i], 170, 60 + i * 36);
        lv_obj_set_size(sl[i], 220, 22);
        lv_slider_set_range(sl[i], 0, 1000);    //小数点 所以设置0-1000
        lv_obj_add_event_cb(sl[i], Sliader_cb, LV_EVENT_VALUE_CHANGED, NULL);
        /* 2.1.3数值 */
        lbv[i] = lbl(scr_alarm, "0.0", &cn_font16);
        lv_obj_set_pos(lbv[i], 410, 60 + i * 36);//同名字就行
    }
     /* 2.2初始化：初次显示缓存里的阈值 */
    thr_sync();

    /* 3. 设置按钮来把他们存入到24c02用于更新阈值 */
     hint = lbl(scr_alarm, "", &cn_font16);
    lv_obj_set_pos(hint, 30, 214);
    const char *Alarm_bn[2] = {"保存", "取消"};
    lv_event_cb_t Alarm_cb[2] = {save_cb, cancel_cb};     //俩回调函数
    for(int i = 0; i < 2; i++)
    {
        lv_obj_t *b = lv_button_create(scr_alarm);
        lv_obj_set_pos(b, 165 + i * 120, 205);
        lv_obj_set_size(b, 65, 40);
        lbl(b, Alarm_bn[i], &cn_font16);
        lv_obj_add_event_cb(b, Alarm_cb[i], LV_EVENT_CLICKED, NULL);
    }
}

/*=============================WIFI=============================*/

/* --- WiFi 名称/密码: 默认取 ota_wifi.h 宏, 界面里可改(改后存在这里) --- */
static char wk_ssid[33] = OTA_WIFI_SSID;
static char wk_pwd[17]  = OTA_WIFI_PWD;
static void data_wifi_ssid_set(const char *s) { strncpy(wk_ssid, s, 32); wk_ssid[32] = 0; }
static void data_wifi_pwd_set(const char *s)  { strncpy(wk_pwd, s, 16); wk_pwd[16] = 0; }

/* --- OTA 服务器: 界面显示合并 "IP:PORT", 拆成 IP/端口两份缓存给 OTA 用 --- */
#define OTA_SRV_ADDR      OTA_SRV_DOMAIN ":" OTA_SRV_PORT   /* 192.168.16.101:8000 */
static char wk_srv[32] = OTA_SRV_ADDR;
static char wk_srv_ip[24]  = OTA_SRV_DOMAIN;   /* 拆好的 IP */
static char wk_srv_port[8] = OTA_SRV_PORT;     /* 拆好的 端口 */
static void data_wifi_srv_set(const char *s)
{
    strncpy(wk_srv, s, 31);
    wk_srv[31] = 0;
    /* "IP:PORT" 按 ':' 拆开 */
    char *colon = strchr(wk_srv, ':');
    if(colon)
    {
        strncpy(wk_srv_ip, wk_srv, colon - wk_srv);
        wk_srv_ip[colon - wk_srv] = 0;
        strncpy(wk_srv_port, colon + 1, 7);
        wk_srv_port[7] = 0;
    }
}
/* --- 给 OTA 状态机取界面改过的值 --- */
char *data_wifi_ssid_get(void)     { return wk_ssid; }
char *data_wifi_pwd_get(void)      { return wk_pwd; }
char *data_wifi_srv_ip_get(void)   { return wk_srv_ip; }
char *data_wifi_srv_port_get(void) { return wk_srv_port; }

/**
 * @description: 键盘状态回调
 * @param {lv_event_t} *e
 * @return {*}
 */
static void ta_cb(lv_event_t *e)
{
    switch(lv_event_get_code(e))
    {
    case LV_EVENT_FOCUSED:                           //点输入框 -> 弹出软键盘
        lv_keyboard_set_textarea(kb, lv_event_get_target_obj(e));
        lv_obj_move_foreground(kb);                  ///键盘置顶, 不被按钮挡住
        lv_obj_remove_flag(kb, LV_OBJ_FLAG_HIDDEN);
        break;
    case LV_EVENT_DEFOCUSED:                         /* 焦点去了别的控件 -> 收起 */
        lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
        break;
    case LV_EVENT_VALUE_CHANGED:                     /* 存到数据接口 */
    {
        lv_obj_t *tg = lv_event_get_target_obj(e);
        if(tg == ta_ssid)     data_wifi_ssid_set(lv_textarea_get_text(ta_ssid));
        else if(tg == ta_srv) data_wifi_srv_set(lv_textarea_get_text(ta_srv));
        else                  data_wifi_pwd_set(lv_textarea_get_text(ta_pwd));
        break;
    }
    }
}

/**
 * @description: (圆按钮: 灰=**** 蓝=明文)
 * @param {lv_event_t} *e
 * @return {*}
 */
static void pwd_show_cb(lv_event_t *e)
{
    lv_obj_t *ta = lv_event_get_user_data(e);
    /* 现在是明文/密码 */
    int show = lv_textarea_get_password_mode(ta);
    /* 来回切换明文/密码 */
    lv_textarea_set_password_mode(ta, !show);
    /* 设置颜色 */
    lv_obj_set_style_bg_color(lv_event_get_target(e),show ? lv_palette_main(LV_PALETTE_BLUE) : lv_palette_main(LV_PALETTE_GREY), 0);
}

/**
 * @description: 创建输入框 事件共用 ta_cb 密码框(显示****)
 * @param {lv_obj_t} *p
 * @param {int} x
 * @param {int} y
 * @param {int} pwd
 * @param {int} len 最大字符数
 * @param {char} *init
 * @return {*}
 */
static lv_obj_t *mk_ta(lv_obj_t *p, int x, int y, int pwd, int len, const char *init)
{
    /* 创建文本区 */
    lv_obj_t *t = lv_textarea_create(p);
    lv_obj_set_pos(t, x, y);
    lv_obj_set_size(t, 210, 32);
    lv_obj_set_style_text_font(t, &cn_font16, 0);
    lv_textarea_set_one_line(t, true); //只能一行
    lv_textarea_set_max_length(t, len);
    if(pwd)
    {
        /*文本区密码模式*/
        lv_textarea_set_password_mode(t, true);
        /*设置内容显示*/
        lv_textarea_set_password_bullet(t, "*");
    }
    if(init)//初始化内容 也就是我定义的变量
    {
        lv_textarea_set_text(t, init);
    }
    lv_obj_add_event_cb(t, ta_cb, LV_EVENT_FOCUSED, NULL);
    lv_obj_add_event_cb(t, ta_cb, LV_EVENT_DEFOCUSED, NULL);
    lv_obj_add_event_cb(t, ta_cb, LV_EVENT_VALUE_CHANGED, NULL);
    return t;
}


/**
 * @description: 开始升级
 * @param {lv_event_t} *e
 * @return {*}
 */
static void start_ota_cb(lv_event_t *e)
{
    if(data_ota_stat() == 0 || data_ota_stat() == 3)
    {
        ota_Statemachine_start();
    }
}

/**
 * @description: 碰到父类scr_wifi即空白处就收起键盘
 * @param {lv_event_t} *e
 * @return {*}
 */
static void wifi_bg_cb(lv_event_t *e)
{
    lv_obj_remove_state(ta_ssid, LV_STATE_FOCUSED);//取消聚焦；比如会有一条斜杠一直在那里闪烁
    lv_obj_remove_state(ta_pwd, LV_STATE_FOCUSED);
    lv_obj_remove_state(ta_srv, LV_STATE_FOCUSED);
    lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
}

/**
 * @description: WiFi: 信息+密码+升级按钮
 * @return {*}
 */
static void WIFI(void)
{
    scr_wifi = lv_obj_create(NULL);
    lv_obj_set_pos(lbl(scr_wifi, "WiFi", &cn_font16), 16, 12);

    /* 1.第1、2、3行: WiFi 名称 + 密码 + 服务器(密码/服务器显示 ****) */
    /* 1.1名称*/
    lv_obj_set_pos(lbl(scr_wifi, "WiFi 名称", &cn_font16), 20, 58);
    ta_ssid = mk_ta(scr_wifi, 170, 54, 0, 32, OTA_WIFI_SSID);
    /* 1.2 密码 */
    lv_obj_set_pos(lbl(scr_wifi, "WiFi 密码", &cn_font16), 20, 92);
    ta_pwd = mk_ta(scr_wifi, 170, 88, 1, 16, OTA_WIFI_PWD);
    /* 1.2.1密码框右侧: 显示/隐藏圆按钮(无文字) */
    btn_eye = lv_button_create(scr_wifi);
    lv_obj_set_pos(btn_eye, 392, 92);
    lv_obj_set_size(btn_eye, 24, 24);
    lv_obj_set_style_radius(btn_eye, LV_RADIUS_CIRCLE, 0);  //圆形
    lv_obj_set_style_bg_color(btn_eye, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_set_style_shadow_opa(btn_eye, LV_OPA_TRANSP, 0); //透明度
    lv_obj_add_event_cb(btn_eye, pwd_show_cb, LV_EVENT_CLICKED, ta_pwd);
    /* 1.3 服务器 */
    lv_obj_set_pos(lbl(scr_wifi, "IP+Port", &cn_font16), 20, 126);
    ta_srv = mk_ta(scr_wifi, 170, 122, 1, 32, OTA_SRV_ADDR);
    /* 1.2IP+Port 框右侧: 显示/隐藏圆按钮(无文字) */
    btn_eye_s = lv_button_create(scr_wifi);
    lv_obj_set_pos(btn_eye_s, 392, 126);
    lv_obj_set_size(btn_eye_s, 24, 24);
    lv_obj_set_style_radius(btn_eye_s, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(btn_eye_s, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_set_style_shadow_opa(btn_eye_s, LV_OPA_TRANSP, 0);
    lv_obj_add_event_cb(btn_eye_s, pwd_show_cb, LV_EVENT_CLICKED, ta_srv);

    /* 2.第4行: 运行内存 + 存储空间(同一行, 改 ui.h 宏) */
    lv_obj_t *n3 = lbl(scr_wifi, "", &cn_font16);
    lv_label_set_text_fmt(n3, "运行内存: %d + %d KB  存储空间: %dKB + %d MB",
                          RAM_INT, RAM_EXT, FLASH_INT, FLASH_EXT);
    lv_obj_set_pos(n3, 20, 164);

    /* 3.第5行: 连接状态 */
    lv_obj_set_pos(lbl(scr_wifi, "连接状态:", &cn_font16), 20, 200);
    lb_stat = lbl(scr_wifi, "空闲", &cn_font16);
    lv_obj_set_pos(lb_stat, 110, 200);

    /* 4.升级按钮 */
    btn_up = lv_button_create(scr_wifi);
    lv_obj_set_pos(btn_up, 230, 192);
    lv_obj_set_size(btn_up, 85, 40);
    lbl(btn_up, "ota升级", &cn_font16);
    lv_obj_add_event_cb(btn_up, start_ota_cb, LV_EVENT_CLICKED, NULL);
    /* 升级进度(红色, 显示 ota_progress_str) */
    lb_prog = lbl(scr_wifi, ota_progress_str, &cn_font16);
    lv_obj_set_pos(lb_prog, 355, 205);
    lv_obj_set_style_text_color(lb_prog, C_RED, 0);

    /* 5. 软键盘 */
    kb = lv_keyboard_create(scr_wifi);
    lv_keyboard_set_textarea(kb, ta_pwd);   //绑定
    lv_obj_align(kb, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);    //藏起来
    lv_obj_add_event_cb(scr_wifi, wifi_bg_cb, LV_EVENT_CLICKED, NULL);
}

/*=============================底部标签页导航=============================*/
/**
 * @description:按下nm[3] = {"首页", "警告", "WiFi"}; 切换页面回调
 * @param {lv_event_t} *e
 * @return {*}
 */
static void nav_cb(lv_event_t *e)
{
    lv_screen_load(lv_event_get_user_data(e));
}
/**
 * @description: 底部标签页导航
 * @param {lv_obj_t} *scr
 * @param {int} cur
 * @return {*}
 */
static void nav(lv_obj_t *scr, int cur)              /* cur = 这是第几页 */
{
    const char *nm[3] = {"首页", "警告", "WiFi"};
    lv_obj_t *tg[3] = {scr_home, scr_alarm, scr_wifi};

    /* 设置底部白条以及样式 并采用flex分布*/
    lv_obj_t *bar = lv_obj_create(scr);              /* 底部白条 */
    lv_obj_set_pos(bar, 0, 264);
    lv_obj_set_size(bar, 480, 63);

    lv_obj_set_style_bg_color(bar, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(bar, 1, 0);
    lv_obj_set_style_border_color(bar, lv_palette_lighten(LV_PALETTE_GREY, 2), 0);//边框颜色

    lv_obj_set_flex_flow(bar, LV_FLEX_FLOW_ROW);     // flex: 标签均匀排开
    lv_obj_set_flex_align(bar, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    for(int i = 0; i < 3; i++) {
        lv_obj_t *b = lv_button_create(bar);
        lv_obj_set_size(b, 100, 20);                 /* 不设位置, flex 自动排 */
        lv_obj_set_style_bg_opa(b, LV_OPA_TRANSP, 0);    //全透明
        lv_obj_set_style_shadow_opa(b, LV_OPA_TRANSP, 0);//全透明
        lv_obj_t *t = lbl(b, nm[i], &cn_font16);
        lv_obj_center(t);

        if(i == cur)  /* 当前页标签 按下变蓝*/
        {
            lv_obj_set_style_text_color(t, lv_palette_main(LV_PALETTE_BLUE), 0);
        }
        else
        {
            lv_obj_set_style_text_color(t, lv_palette_darken(LV_PALETTE_GREY, 3), 0);
        }
        lv_obj_add_event_cb(b, nav_cb, LV_EVENT_CLICKED, tg[i]);
    }
}

/*=============================500ms 定时器=============================*/
static uint8_t th_stale;     //连续多少次没从 modbus 采集到数据
static float last_t, last_h; //历史温湿度
/**
 * @description: 定时器
 * @param {lv_timer_t} *tm
 * @return {*}
 */
static void ui_timer_cb(lv_timer_t *tm)
{
    /* 1. 读温湿度(没有就用上次值) */
    float t, h;
    if(!data_get_th(&t, &h))
    {
        t = last_t;
        h = last_h;
        th_stale++;
    }
    else
    {
        last_t = t;
        last_h = h;
        th_stale = 0; //采到就清零
    }

    /* 2. 报警页进行逻辑判断: 越界即报警, 6*500ms = 3秒无数据也报警 */
    int alarm = (t > thr[0] || t < thr[1] || h > thr[2] || h < thr[3]);
    if(th_stale >= 6)
    {
        alarm = 1;
    }

    /* 3. 首页大数字(整数运算留1位小数) */
    int vlaue = (int)(t * 10.0f + 0.5f);
    lv_label_set_text_fmt(lb_t, "%d.%d", vlaue / 10, vlaue % 10);
    lbl_color(lb_t,C_BLUE);

    vlaue = (int)(h * 10.0f + 0.5f);
    lv_label_set_text_fmt(lb_h, "%d.%d", vlaue / 10, vlaue % 10);
    lbl_color(lb_h,C_BLUE);

    /* 4. 首页的报警阈值两行(与告警页共用 thr[]) */
    int t0 = (int)(thr[0] * 10.0f + 0.5f), t1 = (int)(thr[1] * 10.0f + 0.5f);
    int h0 = (int)(thr[2] * 10.0f + 0.5f), h1 = (int)(thr[3] * 10.0f + 0.5f);

    lv_label_set_text_fmt(lb_thr_t, "温度下限 - 温度上限    %d.%d - %d.%d℃",t1 / 10, t1 % 10, t0 / 10, t0 % 10);
    lv_label_set_text_fmt(lb_thr_h, "湿度下限 - 湿度上限    %d.%d - %d.%d%%",h1 / 10, h1 % 10, h0 / 10, h0 % 10);

    /* 5. 状态字(正常绿/报警红) */
    const char *s = alarm ? "报警" : "正常";
    lv_label_set_text(st_home, s);
    lbl_color(st_home, alarm ? C_RED : C_GREEN);
    lv_label_set_text(st_alarm, s);
    lbl_color(st_alarm, alarm ? C_RED : C_GREEN);

    /* 6. WiFi 状态 + 升级中按钮置灰 */
    static const char *ws[4] = {"空闲", "连接中", "已连接", "失败"};
    lv_label_set_text(lb_stat, ws[data_ota_stat()]);
    lv_label_set_text(lb_prog, ota_progress_str);   /* 升级进度刷新 */
    int st = data_ota_stat();
    if(st == 0 || st == 3)
    {
        //禁止使用
        lv_obj_remove_state(btn_up, LV_STATE_DISABLED);
    }
    else
    {
        //恢复使用按钮
        lv_obj_add_state(btn_up, LV_STATE_DISABLED);
    }
    /* 7. 报警时闪灯 */
    if(alarm)
    {
        LED0_TOGGLE();
        BEEP_TOGGLE();
    }
    else
    {
        LED0(1);
        BEEP(0);
    }
}

/**
 * @description: UI界面初始化  建3页 -> 加标签栏 -> 显示首页 -> 开定时器
 * @return {*}
 */
void ui_init(void)
{
    data_thr_load();
    Shou_ye();
    Alarm();
    WIFI();
    nav(scr_home, 0);                        /* 每页底部的标签栏 */
    nav(scr_alarm, 1);
    nav(scr_wifi, 2);
    lv_obj_t *all[3] = {scr_home, scr_alarm, scr_wifi};
    for(int i = 0; i < 3; i++)
    {
        lv_obj_remove_flag(all[i], LV_OBJ_FLAG_SCROLLABLE); //不要滚动，不然很难看
    }
    /* 首页 */
    lv_screen_load(scr_home);
    lv_timer_create(ui_timer_cb, 500, NULL);
}
