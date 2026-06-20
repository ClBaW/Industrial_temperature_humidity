#include "rs485.h"

UART_HandleTypeDef rs458_handler;     /* RS485控制句柄(串口) */
uint8_t RS485_rx_buf[RS485_REC_LEN];  /* 接收缓冲, 最大 RS485_REC_LEN 个字节. */
uint8_t RS485_rx_cnt = 0;             /* 接收到的数据长度 */


void RS485_UX_IRQHandler(void)
{
    uint8_t res;

    if (__HAL_UART_GET_FLAG(&rs458_handler, UART_FLAG_ORE) != RESET) /* 接收溢出错误 */
    {
        __HAL_UART_CLEAR_OREFLAG(&rs458_handler); /* 清溢出错误中断标志 */
    }

    if ((__HAL_UART_GET_FLAG(&rs458_handler, UART_FLAG_RXNE) != RESET)) /* 接收到数据 */
    {

        HAL_UART_Receive(&rs458_handler, &res, 1, 1000);

        if (RS485_rx_cnt < RS485_REC_LEN)         /* 缓冲区未满 */
        {
            RS485_rx_buf[RS485_rx_cnt] = res;   /* 记录接收到的值 */
            RS485_rx_cnt++;                       /* 接收数据增加1 */
        }
    }
}


/**
 * @description: RS485初始化函数
 * @param {uint32_t} baudrate   波特率, 根据自己需要设置波特率值
 * @return {*}
 */
void rs485_init(uint32_t baudrate)
{
    /* GPIO 及 时钟配置 */
    RS485_RE_GPIO_CLK_ENABLE(); /* 使能 RS485_RE 脚时钟 */
    RS485_TX_GPIO_CLK_ENABLE(); /* 使能 串口TX脚 时钟 */
    RS485_RX_GPIO_CLK_ENABLE(); /* 使能 串口RX脚 时钟 */
    RS485_UX_CLK_ENABLE();      /* 使能 串口 时钟 */

    GPIO_InitTypeDef gpio_initure;
    gpio_initure.Pin = RS485_TX_GPIO_PIN;
    gpio_initure.Mode = GPIO_MODE_AF_PP;
    gpio_initure.Pull = GPIO_NOPULL;
    gpio_initure.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio_initure.Alternate = GPIO_AF7_USART2;               /* 复用为串口2 */
    HAL_GPIO_Init(RS485_TX_GPIO_PORT, &gpio_initure);       /* 串口TX 脚 模式设置 */

    gpio_initure.Pin = RS485_RX_GPIO_PIN;
    HAL_GPIO_Init(RS485_RX_GPIO_PORT, &gpio_initure);       /* 串口RX 脚 必须设置成输入模式 */

    gpio_initure.Pin = RS485_RE_GPIO_PIN;
    gpio_initure.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_initure.Pull = GPIO_PULLUP;
    gpio_initure.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(RS485_RE_GPIO_PORT, &gpio_initure);


    /* USART 初始化设置 */
    rs458_handler.Instance = RS485_UX;                    /* 选择485对应的串口 */
    rs458_handler.Init.BaudRate = baudrate;               /* 波特率 */
    rs458_handler.Init.WordLength = UART_WORDLENGTH_8B;   /* 字长为8位数据格式 */
    rs458_handler.Init.StopBits = UART_STOPBITS_1;        /* 一个停止位 */
    rs458_handler.Init.Parity = UART_PARITY_NONE;         /* 无奇偶校验位 */
    rs458_handler.Init.HwFlowCtl = UART_HWCONTROL_NONE;   /* 无硬件流控 */
    rs458_handler.Init.Mode = UART_MODE_TX_RX;            /* 收发模式 */
    HAL_UART_Init(&rs458_handler);                        /* 使能对应的串口, 但会调用MSp */
    __HAL_UART_DISABLE_IT(&rs458_handler, UART_IT_TC);

                /* 使能接收中断 */
    __HAL_UART_ENABLE_IT(&rs458_handler, UART_IT_RXNE);     /* 开启接收中断 */
    HAL_NVIC_EnableIRQ(RS485_UX_IRQn);                      /* 使能USART2中断 */
    /* 中断优先级设为6, 落入FreeRTOS管辖底线(≥5), 临界区可屏蔽本中断 */
    HAL_NVIC_SetPriority(RS485_UX_IRQn, 6, 6);

    RS485_RE(0); /* 默认为接收模式 */
}

/**
 * @description: RS485查询接收到的数据
 * @param {uint8_t} *buf 接收缓冲区首地址
 * @param {uint8_t} *len 接收到的数据长度
 * @return {*} 0, 表示没有接收到任何数据 其他, 表示接收到的数据长度
 */
void rs485_receive_data(uint8_t *buf, uint8_t *len)
{
    uint8_t rxlen = RS485_rx_cnt;
    uint8_t i = 0;
    *len = 0;     /* 默认为0 */
    delay_ms(10); /* 等待10ms, 连续超过10ms没有接收到一个数据, 则认为接收结束 */

    taskENTER_CRITICAL();   /* 屏蔽中断(USART2优先级6), 防ISR并发修改缓冲 */
    if (rxlen == RS485_rx_cnt && rxlen) /* 接收到了数据, 且接收完成了 */
    {
        for (i = 0; i < rxlen; i++)
        {
            buf[i] = RS485_rx_buf[i];
        }

        *len = rxlen;        /* 记录本次数据长度 */
        RS485_rx_cnt = 0;    /* 清零 */
    }
    taskEXIT_CRITICAL();
}

