#include "ota_Statemachine.h"
#include "ui.h"

/* ------- 25Q128相关变量设定 ------- */
uint16_t page_nb = 0;                        /* 页号(0~3840, 960KB/256B) */
uint8_t page_buf[256];                       /* 页缓冲: 攒满256B写一页 */
uint16_t page_cnt = 0;                       /* 用于做页缓冲内字节数 */
uint8_t recv_buf[512];                       /* 收流缓冲(stream_read用) */
uint32_t recv_count = 0;                     /* 已接收的总字节数 */

/* ------- 超时相关变量设定 ------- */
uint32_t last_tick = 0;                      /* 最后收到数据的时间(HAL_GetTick)；用于超时判定使用 */
uint8_t wait_tip_done = 0;                    /* 最后一段数据停止1秒后提示下载完成(只提示一次) */

/* ------- 结构体初始化定义 ------- */
ota_state_t ota_state = OTA_STATE_IDLE;      /* 设置初始状态为IDLE */
OTA_FlagTypeDef ota_flag_info;               /* 升级标志 */
uint32_t fw_len_expect = 0;          /* LEN声明长度; 0=没解析到(裸bin走老逻辑) */
uint8_t  len_done = 0;               /* 1=LEN行已解析 */
uint8_t  len_line[16];               /* LEN行跨帧拼接缓冲 */
uint8_t  len_cnt = 0;                /* 已拼接字节数 */
char     ota_progress_str[32] = "";  /* 屏幕进度/结果(UI取用) */


/**
 * @description: 开始一次升级: 擦除W25Q128后进WiFi连接状态
 *               (透传后服务器不间断发流, 擦除必须放在连接之前)
 * @return {*}
 */
static void ota_start_update(void)
{
    uint8_t i;

    /* 1.擦除W25Q128(15个64KB块)*/
    printf("\r\n========== 开始OTA升级 ==========\r\n");
    printf("擦除W25Q128(15个64KB块, 约数秒)...\r\n");
    for (i = 0; i < 15; i++)
    {
        ota_25q128_erase(i);  /* 块0~14 = 960KB;与OTA工程设置APP区大小一致即可 */
    }
    printf("擦除完成\r\n");

    recv_count = 0;
    page_nb = 0;
    page_cnt = 0;
    wait_tip_done = 0;

    /* 2.进入WIFI连接状态 */
    ota_state = OTA_STATE_WIFI_CONNECT;
}

/* --- UI “开始升级”按钮的公共封装 --- */
/* (由于ota_start_update 是静态的；因此呢 UI 通过 data_ota_start 调用它) */
void ota_Statemachine_start(void)
{
    if (ota_state != OTA_STATE_IDLE)
    {
        return;
    }
    ota_start_update();
}

// char ip_buf[32];                             /* IP地址显示缓冲 */

/**
 * @description: OTA状态机 rtos任务使用
 * 状态机设定 ：OTA_STATE_IDLE -> OTA_STATE_WIFI_CONNECT -> OTA_STATE_TCP_CONNECT -> OTA_STATE_PASSTHROUGH 
 * -> OTA_STATE_RECEIVE -> OTA_STATE_FINISH -> OTA_STATE_ERROR -> OTA_STATE_IDLE
 * 
 * 先写25Q128 再写24C02 再复位 必须这个顺序
 * 
 * @return {*}
 */
void ota_Statemachine_while(void)
{
    uint16_t len;

    /* ===============状态机主循环: 根据ota_state分支处理=============== */
    switch (ota_state)
    {
        /* ---------- IDLE:通过UI的“开始升级”按钮触发 ---------- */
        case OTA_STATE_IDLE:
        {
            break;
        }

        /* ---------- 连接WiFi ---------- */
        case OTA_STATE_WIFI_CONNECT:
        {
            printf("连接WiFi: %s ...\r\n", data_wifi_ssid_get());
            if (ota_esp8266_join_ap(data_wifi_ssid_get(), data_wifi_pwd_get()) == OTA_ESP8266_EOK)
            {
                printf("WiFi连接成功\r\n");

                /* --------如果要获取IP地址；就打开-------- */
                // if (ota_esp8266_get_ip(ip_buf) == OTA_ESP8266_EOK)
                // {
                //     printf("IP地址: %s\r\n", ip_buf);
                // }
                
                /* ----------进入TCP连接状态---------- */
                ota_state = OTA_STATE_TCP_CONNECT;
            }
            else/* ----------失败回滚---------- */
            {

                printf("WiFi连接失败,请检查SSID/密码\r\n");
                ota_state = OTA_STATE_ERROR;
            }
            break;
        }

        /* ---------- 连接TCP服务器 ---------- */
        case OTA_STATE_TCP_CONNECT:
        {
            printf("连接服务器...\r\n");
            if (ota_esp8266_connect_tcp_server(data_wifi_srv_ip_get(), data_wifi_srv_port_get()) == OTA_ESP8266_EOK)
            {
                printf("TCP连接成功\r\n");
                ota_state = OTA_STATE_PASSTHROUGH;
            }
            else/* ----------失败回滚---------- */
            {
                printf("TCP连接失败, 请确认XNET已开启监听\r\n");
                ota_state = OTA_STATE_ERROR;
            }
            break;
        }

        /* ---------- 进入透传模式 ---------- */
        case OTA_STATE_PASSTHROUGH:
        {
            if (ota_esp8266_enter_unvarnished() == OTA_ESP8266_EOK)
            {
                printf("已进入透传, 请用XNET发送固件bin, 10秒内未发送将退出\r\n");
                last_tick = HAL_GetTick();
                ota_state = OTA_STATE_RECEIVE;
            }
            else/* ----------失败回滚---------- */
            {
            
                printf("进入透传模式失败\r\n");
                ota_state = OTA_STATE_ERROR;
            }
            break;
        }

        /* ---------- 开始接收到来的固件: 1.读串口数据并把喂给evet, 2.做超限回滚/超时判定 ---------- */
        case OTA_STATE_RECEIVE:
        {
            len = ota_esp8266_uart_stream_read(recv_buf, sizeof(recv_buf));
            if (len > 0)
            {
                uint16_t i, data_off = 0;
                last_tick = HAL_GetTick();

                /* LEN行: 跨帧拼接, 收到完整行尾才解析(防拆帧误判) */
                if (len_done == 0)
                {
                    uint8_t  line_end = 0;
                    for (i = 0; (i < len) && (len_cnt < sizeof(len_line)); i++)
                    {
                        len_line[len_cnt++] = recv_buf[i];
                        if (recv_buf[i] == 0x0A)      /* 行尾 */
                        {
                            line_end = 1;
                            break;
                        }
                    }
                    if (line_end || (len_cnt >= sizeof(len_line)))
                    {
                        len_done = 1;
                        if ((len_cnt >= 5) && (len_line[0] == 'L') && (len_line[1] == 'E') &&
                            (len_line[2] == 'N') && (len_line[3] == ':'))
                        {
                            uint32_t val = 0;
                            uint8_t  k;
                            for (k = 4; k < len_cnt; k++)
                            {
                                if ((len_line[k] >= '0') && (len_line[k] <= '9'))
                                {
                                    val = val * 10 + (len_line[k] - '0');
                                }
                                else if ((len_line[k] == 0x0D) || (len_line[k] == 0x0A))
                                {
                                    break;
                                }
                                else
                                {
                                    val = 0;      /* 非法字符 */
                                    break;
                                }
                            }
                            if ((val >= 16) && (val <= OTA_MAX_APP_SIZE))
                            {
                                fw_len_expect = val;
                                printf("LEN: 预期 %u 字节\r\n", (uint32_t)val);
                            }
                            else if (val > OTA_MAX_APP_SIZE)
                            {
                                printf("LEN: %u 预期字节超过 960KB 上限, 无进度百分比\r\n", (uint32_t)val);
                            }
                            else
                            {
                                printf("LEN: %u 数值过小, 无进度百分比\r\n", (uint32_t)val);
                            }
                        }
                        else
                        {
                            printf("未识别LEN行, 将判失败(请发送带长度的ota包)\r\n");
                        }
                        data_off = i + 1;     /* 本帧剩余字节(可能是固件头) */
                    }
                    else
                    {
                        break;   /* 行未收全: 等下一帧, 本帧全部消耗 */
                    }
                }

                ota_Statemachine_evet(recv_buf + data_off, len - data_off); /* 攒页写25Q128 */

                if (recv_count > OTA_MAX_APP_SIZE)
                {
                    printf("\r\n已接收%d字节, 超过APP区上限960KB, 下载失败, 复位回滚\r\n", (int)recv_count);
                    delay_ms(1000);
                    NVIC_SystemReset();
                }

                /* 屏幕进度(每1%更新) */
                if (fw_len_expect != 0)
                {
                    sprintf(ota_progress_str, "%d%%", (int)(recv_count * 100 / fw_len_expect));
                }
                else
                {
                    sprintf(ota_progress_str, "%dKB", (int)(recv_count / 1024));
                }

                /* 收满声明长度 = 完成(不等10秒) */
                if ((fw_len_expect != 0) && (recv_count >= fw_len_expect))
                {
                    printf("\r\n下载完成(共%u字节), 即将写标志复位\r\n", (uint32_t)recv_count);
                    sprintf(ota_progress_str, "\xE5\xB7\xB2\xE9\x80\x9A\xE8\xBF\x87"); /* 已通过 */
                    ota_state = OTA_STATE_FINISH;
                }
            }
            else if (((HAL_GetTick() - last_tick) > 1000) && (recv_count > 0) && (wait_tip_done == 0))
            {
                printf("稍等10秒\r\n");
                wait_tip_done = 1;
            }
            else if ((HAL_GetTick() - last_tick) > OTA_RECV_TIMEOUT)
            {
                if (fw_len_expect != 0)
                {
                    /* 断流: 未收满声明长度 = 判失败, 不写标志 = 不变砖 */
                    printf("\r\n接收超时: 已收%u/预期%u字节, 下载不完整, 判失败\r\n",
                           (uint32_t)recv_count, (uint32_t)fw_len_expect);
                    sprintf(ota_progress_str, "\xE4\xB8\x8B\xE8\xBD\xBD\xE5\xA4\xB1\xE8\xB4\xA5"); /* 下载失败 */
                    ota_state = OTA_STATE_ERROR;
                }
                else
                {
                    /* 无LEN: 一律判失败, 防误判 */
                    printf("接收超时: 未收到LEN行, 判失败(请发送ota包)\r\n");
                    sprintf(ota_progress_str, "\xE4\xB8\x8B\xE8\xBD\xBD\xE5\xA4\xB1\xE8\xB4\xA5"); /* 下载失败 */
                    ota_state = OTA_STATE_ERROR;
                }
            }
            break;
        }

        /* ---------- 关键一步：下载完成: 写24C02标志, 打印, 复位 ---------- */
        case OTA_STATE_FINISH:
        {
            /* 页缓冲可能还有不足256B的尾数据；由于呢得凑满一页所以呢补0xFF写掉 */
            if (page_cnt > 0)
            {
                memset(&page_buf[page_cnt], 0xFF, sizeof(page_buf) - page_cnt);
                ota_25q128_pagewrite(page_buf, page_nb);
            }

            /* 写升级标志到24C02(地址0, 结构体与Bootloader工程OTA/APP/OTA/ota.h一致) */
            memset(&ota_flag_info, 0, sizeof(ota_flag_info));
            ota_flag_info.OTA_up_Flag = OTA_FLAG;                /* 0xAABBAABB */
            ota_flag_info.Firelen[0] = (recv_count + 3) & ~3UL;  /* 固件长度, 4字节对齐 */
            sprintf((char *)ota_flag_info.OTA_Version, OTA_VERSION_STRING);
            ota_24C02_WriteData(0, (uint8_t *)&ota_flag_info, sizeof(ota_flag_info));

            printf("\r\n下载完毕(共%d字节), 即将重启, Bootloader将把固件烧入APP区\r\n", (int)recv_count);
            delay_ms(1000);

            /* 软件重启 */
            NVIC_SystemReset();
            break;
        }

        /* ---------- 出错: 退出透传后打印回待机, 绝不写升级标志 ---------- */
        case OTA_STATE_ERROR:
        {
            ota_esp8266_ensure_at_mode();  /* 退出透传+关TCP: 否则模块吞AT指令, 重试必失败 */
            printf("OTA升级失败, 回到待机，请按屏幕重新连接\r\n");
            ota_state = OTA_STATE_IDLE;
            break;
        }
    }
}

/**
 * @description: 收到数据做什么
 *               数据攒入256B页缓冲, 攒满写一页到W25Q128；才开始写
 * @param {uint8_t*}  data    接收数据
 * @param {uint16_t}  datalen 数据长度
 * @return {*}
 */
void ota_Statemachine_evet(uint8_t *data, uint16_t datalen)
{
    uint16_t i;

    for (i = 0; i < datalen; i++)
    {
        page_buf[page_cnt] = data[i];   //放入对应位置；直到放到满
        page_cnt++;                     //下一字节
        recv_count++;                   //接收到总的字节数，用于打印看进度

        if (page_cnt >= 256) /* 攒满一页, 写入W25Q128 */
        {
            ota_25q128_pagewrite(page_buf, page_nb);
            page_nb++;     //下一页
            page_cnt = 0;  //清空页缓冲

            /* 每10KB
            recv_count/1024 = 当前接收每10KB数
            打印一次已接收KB数 */
            if ((recv_count % 10240) == 0)  //每10kb来一次
            {
                printf("已经接收%dKB(写入到25Q128)\r\n", (int)(recv_count / 1024));
            }
        }
    }
}

/**
 * @description: OTA初始化入口(main.c只调用这一个函数), 所有外设初始化汇总
 * @return {*}
 */
void ota_Statemachine_init(void)
{

    __enable_irq(); /* 必须要！！！！！否则全部中断等于摆设；恢复全局中断(Bootloader跳转前关闭了它) */

    /* 必须！！！防御性重设: 与 system_stm32f4xx.c 的 VECT_TAB_OFFSET(0x10000) 同值 ；其实VECT_TAB_OFFSET已经设置了；再次写防止失效*/
    SCB->VTOR = 0x08010000;

    ota_24c02_init();       /* 初始化AT24C02 */
    ota_25q128_init();      /* 初始化W25Q128 */
    ota_esp8266_init();     /* 初始化ESP8266(硬件复位+AT测试+设置Station模式) */
}

