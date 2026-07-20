#ifndef OTA_25Q128_H   
#define OTA_25Q128_H

#include "./BSP/NORFLASH/norflash.h"
#include "./SYSTEM/usart/usart.h"

void ota_25q128_init(void);

void ota_25q128_erase(uint32_t baddr); /* 块擦除(64KB) */

void ota_25q128_pagewrite(uint8_t *wbuff, uint16_t pagenb); /* 页编程(256字节) */

void ota_25q128_enable(void);                           /* 写使能 */
void ota_25q128_read(uint8_t *rbuf, uint32_t addr, uint32_t datalen); /* 读数据 */



#endif // OTA_25Q128_H
