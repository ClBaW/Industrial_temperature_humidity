#ifndef OTA_H   
#define OTA_H

#include "ota_uasrt.h"
#include "ota_25q128.h"
#include"ota_flash.h"
#include"ota_24c02.h"
#include "ota_boot.h"

/*OTA 分区配置 (F407 内部Flash 1MB, 12扇区)
    bootloader: 0x08000000~0x08007FFF (32KB, 扇区0~1)
    flag:       0x08008000~0x0800BFFF (16KB, 扇区2)
    舍去:       0x0800C000~0x0800FFFF (16KB, 扇区3)
    app:        0x08010000~0x080FFFFF (960KB, 扇区4~11)

*/
#define OTA_page_size 1024  /* 一次1Kb 从25Q128读/写入内部Flash的字节数 越小越细腻,越大越快,网络好可以写大一点 */


#define OTA_BOOT_Start_Addr 0x08000000          /* Bootloader起始地址 */
#define OTA_BOOT_SIZE       0x8000              /* Bootloader大小: 32KB (扇区0~1) */
#define OTA_FLAG            0xAABBAABB          /* 升级标志值, 0xAABB1234表示有新固件, 其他值表示无新固件 */

// #define OTA_FLAG_Start_Addr 0x08008000          /* 升级标志区: 扇区2, 独立16KB */
// #define OTA_FLAG_Sector     2                   /* flag所在扇区号 */


#define OTA_APP_Start_Addr  0x08010000          /* App区起始: 扇区4 */
#define OTA_APP_Sector      4                   /* App起始扇区号 */
#define OTA_APP_SECTOR_NUM  8                   /* App占用扇区数: 4~11，也就是擦除扇区数，根据程序需求可以改 */ 
// #define OTA_APP_SIZE        0xF0000             /* App区大小: 960KB */


#define OTA_FW_Start_Addr   0x000000            /* W25Q128固件暂存起始地址 */
#define OTA_FW_SIZE         0xF0000             /* 固件暂存区大小: 960KB 0——14块block 也就是15块block*/



/* ============标志位============*/
#define OTA_updata_APP_Flag 0x00000001      //OTA_updata_APP_Flag置位,APP更新程序
#define SET_VERSION_Flag    0x00000002
#define Download_outflash   0x00000004
#define ADD_outflash_TO_APP 0x00000008


typedef struct
{
    uint32_t  OTA_up_Flag; 
    uint32_t  Firelen[11];      //11 ： 12*4字节(uint32_t) =48字节 = 3页24c02    
    uint8_t   OTA_Version[32];  //OTA版本号 VER-1.0.0-2026/8/20-12:00
}OTA_FlagTypeDef;



#define OTA_FLAG_SIZE sizeof(OTA_FlagTypeDef)  /* OTA标志结构体大小 */


typedef struct
{
    uint8_t Updatabuff[OTA_page_size];          //更新APP区,用于保存从25Q128中读取的数据
    uint32_t NM25Q128_BlockNumber;              //记录是哪个25q128块中读取到的数据
}UpDataAPP_TypeDef;


extern OTA_FlagTypeDef ota_flag;
extern UpDataAPP_TypeDef UpDataAPP; 
extern uint32_t Boot_State_flag;

void ota_init(void);
void ota_main(void);
void ota_hardware_test(void);
void ota_while(void);


#endif // OTA_H
