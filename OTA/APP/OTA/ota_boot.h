#ifndef OTA_BOOT_H
#define OTA_BOOT_H

#include "stdio.h"
#include "ota.h"

typedef void (*LOAD_APP)(void);

void ota_boot_Jump(void);

uint8_t ota_boot_Enter(uint8_t timeout);

void ota_boot_Event(uint8_t * data, uint16_t datalen);

void ota_boot_information(void);

void ota_Load_app(uint32_t addr);

void ota_boot_clear(void);

uint16_t Xmodem_CRC16(uint8_t *data,uint16_t datalen);

#endif // OTA_BOOT_H
