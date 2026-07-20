#include "ota_wifi_usart.h"

uint8_t ota_esp8266_uart_rx_buf[OTA_ESP8266_UART_RX_BUF_SIZE]; /* 环形接收缓冲 */
ota_esp8266_uart_CB U3CB;                                      /* 接收控制块 */


UART_HandleTypeDef g_esp8266_uart_handle; /* 串口句柄 */
/**
 * @description: wifi模块串口初始化 
 * @param {uint32_t} baudrate 波特率
 * @return {*}
 */
void ota_esp8266_uart_init(uint32_t baudrate)
{
    GPIO_InitTypeDef gpio_init_struct; /* GPIO初始化结构体 */

    OTA_ESP8266_UART_TX_GPIO_CLK_ENABLE(); /* 使能UART TX引脚时钟 */
    OTA_ESP8266_UART_RX_GPIO_CLK_ENABLE(); /* 使能UART RX引脚时钟 */
    OTA_ESP8266_UART_CLK_ENABLE();         /* 使能UART时钟 */

    gpio_init_struct.Pin = OTA_ESP8266_UART_TX_GPIO_PIN;             /* UART TX引脚 */
    gpio_init_struct.Mode = GPIO_MODE_AF_PP;                         /* 复用推挽输出 */
    gpio_init_struct.Pull = GPIO_NOPULL;                             /* 无上下拉 */
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;                   /* 高速 */
    gpio_init_struct.Alternate = OTA_ESP8266_UART_TX_GPIO_AF;        /* 复用为USART3 */
    HAL_GPIO_Init(OTA_ESP8266_UART_TX_GPIO_PORT, &gpio_init_struct); /* 初始化UART TX引脚 */

    gpio_init_struct.Pin = OTA_ESP8266_UART_RX_GPIO_PIN;             /* UART RX引脚 */
    gpio_init_struct.Mode = GPIO_MODE_AF_PP;                         /* 复用推挽输出 */
    gpio_init_struct.Pull = GPIO_NOPULL;                             /* 无上下拉 */
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;                   /* 高速 */
    gpio_init_struct.Alternate = OTA_ESP8266_UART_RX_GPIO_AF;        /* 复用为USART3 */
    HAL_GPIO_Init(OTA_ESP8266_UART_RX_GPIO_PORT, &gpio_init_struct); /* 初始化UART RX引脚 */

    g_esp8266_uart_handle.Instance = OTA_ESP8266_UART_INTERFACE;    /* USART3 */
    g_esp8266_uart_handle.Init.BaudRate = baudrate;                 /* 波特率 */
    g_esp8266_uart_handle.Init.WordLength = UART_WORDLENGTH_8B;     /* 字长为8位数据格式 */
    g_esp8266_uart_handle.Init.StopBits = UART_STOPBITS_1;          /* 一个停止位 */
    g_esp8266_uart_handle.Init.Parity = UART_PARITY_NONE;           /* 无奇偶校验位 */
    g_esp8266_uart_handle.Init.HwFlowCtl = UART_HWCONTROL_NONE;     /* 无硬件流控 */
    g_esp8266_uart_handle.Init.Mode = UART_MODE_TX_RX;              /* 收发模式 */
    g_esp8266_uart_handle.Init.OverSampling = UART_OVERSAMPLING_16; /* 16倍过采样 */
    HAL_UART_Init(&g_esp8266_uart_handle);                          /* 初始化USART3 */

    HAL_NVIC_SetPriority(OTA_ESP8266_UART_IRQn, 0, 0);          /* 抢占优先级0, 子优先级0 */
    HAL_NVIC_EnableIRQ(OTA_ESP8266_UART_IRQn);                  /* 使能UART中断通道 */
    __HAL_UART_ENABLE_IT(&g_esp8266_uart_handle, UART_IT_RXNE); /* 使能RXNE接收中断: 逐字节接收 */

    /* 关键：读写计数清零 */
    U3CB.URxWriteCnt = 0;
    U3CB.URxReadCnt = 0;
}

/**
 * @description: 环形接收缓冲读取
 * @param {uint8_t} *buf    要读取的缓冲区数组
 * @param {uint16_t} maxlen 要读取的最大长度
 * @return {*}
 */
uint16_t ota_esp8266_uart_stream_read(uint8_t *buf, uint16_t maxlen)
{
    uint16_t avail = (uint16_t)(U3CB.URxWriteCnt - U3CB.URxReadCnt); /* 环形计数差 = 可读字节数 */
    uint16_t n = (avail > maxlen) ? maxlen : avail;                  /* 取最小；不能超过maxlen */
    uint16_t i;

    for (i = 0; i < n; i++)
    {
        buf[i] = ota_esp8266_uart_rx_buf[(U3CB.URxReadCnt + i) & (OTA_ESP8266_UART_RX_BUF_SIZE - 1)];
    }
    U3CB.URxReadCnt += n;

    return n;
}

/**
 * @description: wifi模块中断 清除溢出错误中断标志;以及读取数据放入ota_esp8266_uart_rx_buf
 * @return {*}
 */
void USART3_IRQHandler(void)
{
    if (__HAL_UART_GET_FLAG(&g_esp8266_uart_handle, UART_FLAG_ORE) != RESET) /* 接收溢出错误 */
    {
        __HAL_UART_CLEAR_OREFLAG(&g_esp8266_uart_handle); /* 清溢出错误中断标志 */
    }

    if (__HAL_UART_GET_FLAG(&g_esp8266_uart_handle, UART_FLAG_RXNE) != RESET) /* 接收到一个字节 */
    {
        /* 写入环形缓冲: &(2048-1)取模, URxWriteCnt为累计计数(ISR写) */
        ota_esp8266_uart_rx_buf[U3CB.URxWriteCnt & (OTA_ESP8266_UART_RX_BUF_SIZE - 1)] =
            (uint8_t)(g_esp8266_uart_handle.Instance->DR & 0x1FF);  //清除高位；获取8位数据放入我的缓冲区当中
        U3CB.URxWriteCnt++;
    }
}

/**
 * @description: printf
 * @param {char} *fmt
 * @return {*}
 */
void ota_esp8266_uart_printf(const char *fmt, ...)
{
    static char tx_buf[64]; /* 发送缓冲 */
    va_list ap;
    uint16_t len;

    va_start(ap, fmt);
    vsprintf((char *)tx_buf, fmt, ap);
    va_end(ap);

    len = strlen((const char *)tx_buf);
    HAL_UART_Transmit(&g_esp8266_uart_handle, (uint8_t *)tx_buf, len, HAL_MAX_DELAY); /* 查询发送 */
}
