#include "ota_uasrt.h"
#if 1
#if (__ARMCC_VERSION >= 6010050)                                /* 使用AC6编译器时 */
__asm(".global __use_no_semihosting\n\t");                      /* 不适用于半主机模式 */
__asm(".global __ARM_use_no_argv \n\t");                        /* AC6不需要将main函数作为参数格式 */

#else
/* 使用AC5编译器时, 要在这里定义__FILE 及 使能不使用半主机模式 */
#pragma import(__use_no_semihosting)

struct __FILE
{
    int handle;
};

#endif
/* 不使用半主机模式时需要重定义_ttywrch\_sys_exit\_sys_command_string三个函数, 同时兼容AC6和AC5模式 */
int _ttywrch(int ch)
{
    ch = ch;
    return ch;
}

/* _sys_exit()避免使用半主机模式 */
void _sys_exit(int x)
{
    x = x;
}

char *_sys_command_string(char *cmd, int len)
{
    return NULL;
}

/* FILE 在 stdio.h中有定义. */
FILE __stdout;

/* 重定向fputc函数, printf最终通过此函数发送字符数据 (OTA USART1) */
int fputc(int ch, FILE *f)
{
    while ((OTA_USART_UX->SR & 0X40) == 0);            /* 等待发送完成一个字符 */
    OTA_USART_UX->DR = (uint8_t)ch;                    /* 需要发送的字符 ch 写入到DR寄存器 */
    return ch;
}
#endif



/*====================================================以下ota内容====================================================*/
uint8_t ota_uasrt1_rx_buf[OTA_UASRT1_RX_BUF_SIZE]; /* 接收缓冲区 */
ota_uasrt_CB U1CB;                                 /* 串口接收控制块 */

UART_HandleTypeDef g_uart1_handle;    /* UART句柄 */
DMA_HandleTypeDef g_uart1_dma_handle; /* DMA句柄(USART1_RX) */

/**
 * @description: ota的串口初始化
 * @param {uint32_t} baudrate 波特率
 * @return {*}
 */
void ota_uasrt_init(uint32_t baudrate)
{
    GPIO_InitTypeDef gpio_init_struct; /* GPIO初始化结构体 */
    OTA_UART_TX_GPIO_CLK_ENABLE();     /* 使能UART TX引脚时钟 */
    OTA_UART_RX_GPIO_CLK_ENABLE();     /* 使能UART RX引脚时钟 */
    OTA_UART_CLK_ENABLE();             /* 使能UART时钟 */

    gpio_init_struct.Pin = OTA_UART_TX_GPIO_PIN;             /* UART TX引脚 */
    gpio_init_struct.Mode = GPIO_MODE_AF_PP;                 /* 复用推挽输出 */
    gpio_init_struct.Pull = GPIO_NOPULL;                     /* 无上下拉 */
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;           /* 高速 */
    gpio_init_struct.Alternate = OTA_UART_TX_GPIO_AF;        /* 复用为USART1 */
    HAL_GPIO_Init(OTA_UART_TX_GPIO_PORT, &gpio_init_struct); /* 初始化UART TX引脚 */

    gpio_init_struct.Pin = OTA_UART_RX_GPIO_PIN;             /* UART RX引脚 */
    gpio_init_struct.Mode = GPIO_MODE_AF_PP;                 /* 复用推挽输出 */
    gpio_init_struct.Pull = GPIO_NOPULL;                     /* 无上下拉 */
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;           /* 高速 */
    gpio_init_struct.Alternate = OTA_UART_RX_GPIO_AF;        /* 复用为USART1 */
    HAL_GPIO_Init(OTA_UART_RX_GPIO_PORT, &gpio_init_struct); /* 初始化UART RX引脚 */

    /* 1. USART1 外设初始化 */
    g_uart1_handle.Instance = OTA_USART_UX;              /* USART1 */
    g_uart1_handle.Init.BaudRate = baudrate;             /* 波特率 */
    g_uart1_handle.Init.WordLength = UART_WORDLENGTH_8B; /* 字长为8位数据格式 */
    g_uart1_handle.Init.StopBits = UART_STOPBITS_1;      /* 一个停止位 */
    g_uart1_handle.Init.Parity = UART_PARITY_NONE;       /* 无奇偶校验位 */
    g_uart1_handle.Init.HwFlowCtl = UART_HWCONTROL_NONE; /* 无硬件流控 */
    g_uart1_handle.Init.Mode = UART_MODE_TX_RX;          /* 收发模式 */
    HAL_UART_Init(&g_uart1_handle);                      /* 初始化USART1 */
    HAL_NVIC_SetPriority(OTA_UART_IRQn, 0, 0);           /* 抢占优先级0, 子优先级0 */
    HAL_NVIC_EnableIRQ(OTA_UART_IRQn);                   /* 使能UART中断通道 */

    /* 2. DMA2 Stream2 初始化 (USART1_RX: DMA2_Stream2, 通道4), 单次模式 */
    __HAL_RCC_DMA2_CLK_ENABLE(); /* DMA2时钟使能 */

    g_uart1_dma_handle.Instance = DMA2_Stream2;                        /* DMA2 Stream2 */
    g_uart1_dma_handle.Init.Channel = DMA_CHANNEL_4;                   /* USART1_RX 对应通道4 */
    g_uart1_dma_handle.Init.Direction = DMA_PERIPH_TO_MEMORY;          /* 外设到内存 */
    g_uart1_dma_handle.Init.PeriphInc = DMA_PINC_DISABLE;              /* 外设地址不变 */
    g_uart1_dma_handle.Init.MemInc = DMA_MINC_ENABLE;                  /* 内存地址自增 */
    g_uart1_dma_handle.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE; /* 外设数据宽度8位 */
    g_uart1_dma_handle.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;    /* 内存数据宽度8位 */
    g_uart1_dma_handle.Init.Mode = DMA_NORMAL;                         /* 正常模式, 不循环 */
    g_uart1_dma_handle.Init.Priority = DMA_PRIORITY_HIGH;              /* 高优先级 */
    g_uart1_dma_handle.Init.FIFOMode = DMA_FIFOMODE_DISABLE;           /* 关闭FIFO */
    HAL_DMA_Init(&g_uart1_dma_handle);                                 /* 初始化DMA */
    __HAL_LINKDMA(&g_uart1_handle, hdmarx, g_uart1_dma_handle);        /* DMA关联到UART */

    HAL_UART_Receive_DMA(&g_uart1_handle, ota_uasrt1_rx_buf, OTA_UASRT1_RX_MAX + 1);
    __HAL_UART_ENABLE_IT(&g_uart1_handle, UART_IT_IDLE); /* 使能空闲中断 */
    __HAL_UART_CLEAR_IDLEFLAG(&g_uart1_handle);          /* 清除使能瞬间的空闲标志 */
    ota_uasrt_Rx_ptr_init();                             /* 初始化接收指针 */
}

/**
 * @description: 环形缓冲区初始化 清空变量
 * @return {*}
 */
void ota_uasrt_Rx_ptr_init(void)
{
    U1CB.URxData_IN = &U1CB.URxDataPtr[0];        /* 接收指针指向接收缓冲区首地址 */
    U1CB.URxData_OUT = &U1CB.URxDataPtr[0];       /* 发送指针指向接收缓冲区首地址 */
    U1CB.URxData_END = &U1CB.URxDataPtr[NUM - 1]; /* 接收缓冲区尾地址 */

    U1CB.URxData_IN->start = ota_uasrt1_rx_buf; /* 接收缓冲区起始地址 */
    U1CB.URxCount = 0;                          /* 接收计数清零 */
}

void OTA_UART_IRQHandler(void)
{

    if (__HAL_UART_GET_FLAG(&g_uart1_handle, UART_FLAG_IDLE) != RESET)
    {
        /* 本次长度 = 重启 DMA 时设置的总长度 - DMA 剩余未搬完的字节数*/
        uint16_t len = (OTA_UASRT1_RX_MAX + 1) - __HAL_DMA_GET_COUNTER(&g_uart1_dma_handle);  

        /* 清除空闲中断标志 */
        __HAL_UART_CLEAR_IDLEFLAG(&g_uart1_handle); 

        if (len > 0) /* 收到0字节帧 */
        {
            U1CB.URxCount = (U1CB.URxCount + len) % OTA_UASRT1_RX_BUF_SIZE;  /* 取模防溢出: 到2048自动回0, 防止start指针越界 */
            U1CB.URxData_IN->end = &ota_uasrt1_rx_buf[(U1CB.URxCount - 1) % OTA_UASRT1_RX_BUF_SIZE]; /* 更新接收数组尾部地址 */
            U1CB.URxData_IN++;                                                                       /* 写入指针加1 */
            if (U1CB.URxData_IN == U1CB.URxData_END)
            {
                U1CB.URxData_IN = &U1CB.URxDataPtr[0]; /* 写入指针指向接收缓冲区头部地址 */
            }
            if ((OTA_UASRT1_RX_BUF_SIZE - U1CB.URxCount) >= OTA_UASRT1_RX_MAX)
            {
                U1CB.URxData_IN->start = &ota_uasrt1_rx_buf[U1CB.URxCount]; /* 更新接收数组当前起始地址 */
            }
            else
            {
                U1CB.URxData_IN->start = ota_uasrt1_rx_buf; /* 更新接收数组起始地址 */
            }
        }

        HAL_UART_DMAStop(&g_uart1_handle);                                                    /* 关掉DMA */
        HAL_UART_Receive_DMA(&g_uart1_handle, U1CB.URxData_IN->start, OTA_UASRT1_RX_MAX + 1); /* 重新开启DMA接收, 长度=OTA_UASRT1_RX_MAX+1 */
    }
}

/**
 * @description: 串口测试函数
 * @return {*}
 */
void ota_usart_test(void)
{
    uint16_t i;
    if (U1CB.URxData_OUT != U1CB.URxData_IN) /* 接收缓冲区非空 */
    {
        printf("收到%d字节数据", U1CB.URxData_OUT->end - U1CB.URxData_OUT->start + 1); /* 打印接收数据长度 */
        for (i = 0; i < U1CB.URxData_OUT->end - U1CB.URxData_OUT->start + 1; i++)
        {
            printf("%c ", U1CB.URxData_OUT->start[i]); /* 打印接收数据 */
        }
        printf("\r\n\r\n");
        U1CB.URxData_OUT++;                       /* 读取指针加1 */
        if (U1CB.URxData_OUT == U1CB.URxData_END) // 读取指针指向接收缓冲区尾部地址
        {
            U1CB.URxData_OUT = &U1CB.URxDataPtr[0]; // 读取指针指向接收缓冲区头部地址
        }
    }
}

