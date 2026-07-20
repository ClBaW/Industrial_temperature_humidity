#include"ota_24c02.h"

/**
 * @brief       初始化IIC接口
 */
void ota_24c02_init(void)
{
    at24cxx_init(); /* 初始化IIC接口 */
}

/**
 * @brief       M24C02 写一个字节
 * @param       addr  : 目标地址 (0~255)
 * @param       wdata : 要写入的数据
 * @retval      0     : 成功
 *              (at24cxx驱动函数无错误返回, 故固定返回0)
 */
uint8_t ota_24C02_WriteByte(uint8_t addr, uint8_t wdata)
{
    at24cxx_write_one_byte(addr, wdata);    /* 24CXX驱动: 单字节写入, 内部含10ms写周期等待 */
    return 0;
}


/**
 * @brief       M24C02 写数据
 * @param       addr   : 起始地址 (0~255)
 * @param       wdata  : 要写入的数据指针
 * @param       datalen: 要写入的字节数 (0~65535)
 * @retval      0      : 成功
 *              (at24cxx驱动函数无错误返回, 故固定返回0)
 */
uint8_t ota_24C02_WriteData(uint8_t addr, uint8_t *wdata, uint16_t datalen)
{
    at24cxx_write(addr, wdata, datalen);
    return 0;
}


/**
 * @brief       M24C02 页写入
 * @param       addr  : 起始地址 (0~255)
 * @param       wdata : 要写入的数据指针
 * @retval      0     : 成功
 *              (at24cxx驱动函数无错误返回, 故固定返回0)
 */
uint8_t ota_24C02_WritePage(uint8_t addr, uint8_t *wdata)
{
    at24cxx_write(addr, wdata, 16);         /* 24CXX驱动: 单字节依次写入16字节, 无跨页回卷问题 */
    return 0;
}


/**
 * @brief       M24C02 读数据
 * @param       addr   : 起始地址 (0~255)
 * @param       rdata  : 数据存储指针
 * @param       datalen: 要读取的字节数 (0~65535)
 * @retval      0      : 成功
 *              (at24cxx驱动函数无错误返回, 故固定返回0)
 */
uint8_t ota_24C02_ReadData(uint8_t addr, uint8_t *rdata, uint16_t datalen)
{
    at24cxx_read(addr, rdata, datalen);
    return 0;
}

/**
 * @description: 读取24C02中的OTA标志
 * @return {*}
 */
void ota_24C02_ReadOTAFlag(void)
{
    /*1. 先清空结构体 */
    memset(&ota_flag, 0, OTA_FLAG_SIZE);

    /*2. 读取24C02中的OTA标志 */
    ota_24C02_ReadData(0, (uint8_t *)&ota_flag, OTA_FLAG_SIZE);
}


/**
 * @description: 读取24C02中的OTA标志
 * @return {*}
 */
void ota_24C02_WriteOTAFlag(void)
{
    uint8_t i;
    uint8_t *wptr;

    wptr = (uint8_t *)&ota_flag;  

    for(i=0;i < (OTA_FLAG_SIZE / 16);i++)
    {
        ota_24C02_WritePage(i * 16, wptr + i * 16);
        delay_ms(5);
    }
}
