#include"ota_flash.h"

/**
 * @description: 擦除内部FLASH指定扇区
 * @param {uint16_t} start       起始扇区号(0~11)
 * @param {uint16_t} num_sectors 要擦除的扇区数量
 * @return {*}
 */
void ota_flash_eraseflash(uint16_t start, uint16_t num_sectors)
{
    FLASH_EraseInitTypeDef erase;
    uint32_t sector_err = 0;

    if (num_sectors == 0) return;               /* 数量为0, 直接返回 */
    if ((start + num_sectors) > 12) return;      /* 超出F407扇区范围(0~11) */

    erase.TypeErase    = FLASH_TYPEERASE_SECTORS;
    erase.Sector       = start;                 /* 起始扇区号 */
    erase.NbSectors    = num_sectors;           /* 一次擦多个扇区 */
    erase.VoltageRange = FLASH_VOLTAGE_RANGE_3;

    HAL_FLASH_Unlock();     /* 解锁FLASH */
    HAL_FLASHEx_Erase(&erase, &sector_err);     /* 一次搞定 */
    HAL_FLASH_Lock();       /* 重新上锁 */
}

/**
 * @description: 写入内部FLASH指定地址
 * @param {uint32_t} saddr     起始写入地址(必须4字节对齐)
 * @param {uint32_t} *wdata    数据指针
 * @param {uint32_t} wnum      写入数据数量(字节数, 每次写1个字=4字节)  
 * @return {*}
 */
void ota_flash_writeflash(uint32_t saddr, uint32_t *wdata, uint32_t wnum)
{

    /* 1.参数检查 */
    if (wnum == 0) return;                      /* 写入0个字, 直接返回 */
    if (wdata == NULL) return;                  /* 数据指针为空 */
    if ((saddr < 0x08000000) || (saddr > 0x080FFFFF) || (saddr & 0x3))
    {
        return;                                 /* 地址非法或未4字节对齐 */
    }
    if ((uint64_t)saddr + (uint64_t)wnum > 0x08100000) return;  /* 超出Flash范围 */


    /* 2.写入FLASH */
    HAL_FLASH_Unlock();     /* 解锁FLASH */
    while (wnum >= 4)                  /* 按字数循环, 每轮1个字(4字节) */
    {
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, saddr, *wdata) != HAL_OK)
        {
            break;                              /* 写入失败, 退出 */
        }
        saddr += 4;                             /* 下一个地址 */
        wnum -= 4;
        wdata++;
    }
    HAL_FLASH_Lock();       /* 重新上锁 */
}
