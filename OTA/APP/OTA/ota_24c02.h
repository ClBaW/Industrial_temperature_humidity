#ifndef OTA_24C02_H
#define OTA_24C02_H

#include "./BSP/24CXX/24cxx.h"
#include "ota.h"
#include "string.h"
#include "./SYSTEM/delay/delay.h"

void ota_24c02_init(void);

uint8_t ota_24C02_WriteByte(uint8_t addr, uint8_t wdata);

uint8_t ota_24C02_WriteData(uint8_t addr, uint8_t *wdata, uint16_t datalen);

uint8_t ota_24C02_WritePage(uint8_t addr, uint8_t *wdata);

uint8_t ota_24C02_ReadData(uint8_t addr, uint8_t *rdata, uint16_t datalen);

void ota_24C02_ReadOTAFlag(void);

void ota_24C02_WriteOTAFlag(void);

#endif // OTA_24C02_H
