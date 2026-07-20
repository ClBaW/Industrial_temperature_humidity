#include"ota_25q128.h"
#include "./BSP/SPI/spi.h"


/**
 * @description: nw25q128初始化
 * @return {*}
 */
void ota_25q128_init(void)
{
    norflash_init();
}

/**
 * @description: nw25q128写使能
 * @return {*}
 */
void ota_25q128_enable(void)
{
    norflash_write_enable();
}



/**
 * @description: nw25q128 块擦除
 * @param {uint32_t} baddr  块序号(0~255), 对应64KB对齐的起始地址
 * @return {*}
 */
void ota_25q128_erase(uint32_t baddr)
{
    uint32_t addr = baddr * 65536;          /* 块序号转字节地址 */
    if (baddr > 255) return;        /* 越界直接返回 */
    norflash_write_enable();                /* 写使能 */
    while ((norflash_read_sr(1) & 0x01) == 0x01);   /* 等待空闲 */

    NORFLASH_CS(0);
    spi1_read_write_byte(FLASH_BlockErase);         /* 0xD8 块擦除指令 */
    spi1_read_write_byte((addr >> 16) & 0xFF);      /* 地址高字节 */
    spi1_read_write_byte((addr >> 8)  & 0xFF);      /* 地址中字节 */
    spi1_read_write_byte(addr & 0xFF);              /* 地址低字节 */
    NORFLASH_CS(1);
    while ((norflash_read_sr(1) & 0x01) == 0x01);   /* 等待擦除完成 */
}

/**
 * @description: nw25q128页编程(写满一页256字节)
 * @param {uint8_t*} wbuff  数据缓冲区(256字节)
 * @param {uint16_t} pagenb 页序号(0~65535), 对应256字节对齐的地址
 * @return {*}
 */
void ota_25q128_pagewrite(uint8_t *wbuff, uint16_t pagenb)
{
    norflash_write(wbuff, (uint32_t)pagenb * 256, 256);    /* 调norflash公开函数, 写满一页 */
}

/**
 * @description: nw25q128读数据
 * @param {uint8_t*} rbuf   数据存储区
 * @param {uint32_t} addr   起始读取的地址(最大32bit)
 * @param {uint32_t} datalen 要读取的字节数
 * @return {*}
 */
void ota_25q128_read(uint8_t *rbuf, uint32_t addr, uint32_t datalen)
{
    uint32_t i;
    if ((addr > 0xFFFFFF) || (datalen > 0x1000000 - addr)) return;   /* 超出芯片容量, 直接返回 */

    NORFLASH_CS(0);
    spi1_read_write_byte(FLASH_ReadData);       /* 0x03 读数据指令 */
    spi1_read_write_byte((addr >> 16) & 0xFF);  /* 地址高字节 */
    spi1_read_write_byte((addr >> 8)  & 0xFF);  /* 地址中字节 */
    spi1_read_write_byte(addr & 0xFF);          /* 地址低字节 */
    for (i = 0; i < datalen; i++)
    {
        rbuf[i] = spi1_read_write_byte(0xFF);   /* 循环读取 */
    }
    NORFLASH_CS(1);
}
