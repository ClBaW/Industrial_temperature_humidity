#ifndef OTA_FLASH_H
#define OTA_FLASH_H

#include "stdio.h"
#include "./SYSTEM/sys/sys.h"

void ota_flash_eraseflash(uint16_t start, uint16_t num_sectors);  

void ota_flash_writeflash(uint32_t saddr, uint32_t *wdata, uint32_t wnum);

#endif // OTA_FLASH_H
