#include"ota.h"
#include "./BSP/SPI/spi.h"
/*
功能：OTA升级
目前已实现的功能：
1.跳转
2.升级
未实现的功能：
3.串口IAP
4.存储ota版本号到24c02
5.flash数据区管理实现
 */

OTA_FlagTypeDef ota_flag;
UpDataAPP_TypeDef UpDataAPP;

uint32_t Boot_State_flag;
uint32_t i;

void ota_init(void)
{
    ota_uasrt_init(115200);               /* 初始化OTA串口为115200 */
    ota_24c02_init();                     /* 初始化AT24C02 */
    ota_25q128_init();                    /* 初始化NOR FLASH */
}

void ota_main(void)
{
   
    ota_24C02_ReadOTAFlag();              /* 判断前必须先读出24C02升级标志 */
    ota_boot_Jump();                      /* 根据OTA标志判断是否跳转 */
}




void ota_while(void)
{
        delay_ms(10);   
        if (U1CB.URxData_OUT != U1CB.URxData_IN) /* 接收缓冲区不为空 */
        {
            ota_boot_Event(U1CB.URxData_OUT->start, U1CB.URxData_OUT->end - U1CB.URxData_OUT->start + 1);
            U1CB.URxData_OUT++;                       /* 接收指针自增 */
            if (U1CB.URxData_OUT == U1CB.URxData_END) // 接收指针指向接收缓冲区尾地址
            {
                U1CB.URxData_OUT = &U1CB.URxDataPtr[0]; // 接收指针指向接收缓冲区首地址
            }
        }



        /*==================OTA_updata_APP_Flag置位,APP开始更新程序==================*/
        if(Boot_State_flag & OTA_updata_APP_Flag)
        {
            /*=====更新APP=====*/
            printf("==========将写入数据长度为%d==========\r\n",ota_flag.Firelen[0]);
            if(ota_flag.Firelen[0] % 4 == 0)
            {
                __disable_irq();   /* 擦写内部Flash期间关中断；保险代码；没有其实也是可以跑；我用来防止bug发生 */
                ota_flash_eraseflash(OTA_APP_Sector,OTA_APP_SECTOR_NUM);    //擦除APP区 OTA_APP_SECTOR_NUM按需求改
                for(i=0;i < ota_flag.Firelen[0] / OTA_page_size;i++)
                {
                    ota_25q128_read(UpDataAPP.Updatabuff,i*1024,OTA_page_size);
                    ota_flash_writeflash(OTA_APP_Start_Addr + i * OTA_page_size,(uint32_t *)UpDataAPP.Updatabuff,OTA_page_size);
                    /* 写后立即读回对比(诊断烧录是否写坏) */
                    {
                        uint32_t k;
                        uint8_t  bad = 0;
                        for(k = 0; k < OTA_page_size; k++)
                        {
                            if(*(volatile uint8_t *)(OTA_APP_Start_Addr + i * OTA_page_size + k) != UpDataAPP.Updatabuff[k])
                            {
                                bad = 1;
                                break;
                            }
                        }
                        if(bad)
                        {
                            printf("WRITE-FAIL @0x%08X %c%c", (uint32_t)(OTA_APP_Start_Addr + i * OTA_page_size + k), 13, 10);
                        }
                    }
                    /* 每10KB打印一次写入进度 */
                    if ((i % 10) == 9)
                    {
                        printf("已经写入%dKB(从25q128到内部flash)\r\n", (int)((i + 1) * OTA_page_size / 1024));
                    }
                }
                if(ota_flag.Firelen[0] % 1024 != 0)
                {
                    ota_25q128_read(UpDataAPP.Updatabuff,i*1024,ota_flag.Firelen[0] % 1024);
                    ota_flash_writeflash(OTA_APP_Start_Addr + i * OTA_page_size,(uint32_t *)UpDataAPP.Updatabuff,ota_flag.Firelen[0] % 1024);
                }
                __enable_irq();

                if(UpDataAPP.NM25Q128_BlockNumber == 0)
                {
                    ota_flag.OTA_up_Flag = 0x11223344;
                    ota_24C02_WriteOTAFlag();
                }
                printf("更新APP完成\r\n");
                NVIC_SystemReset(); 
            }
            else
            {
                printf("数据长度异常\r\n");
                Boot_State_flag &= ~OTA_updata_APP_Flag;
            }
        }
}


/**
 * @description: 硬件自检: 24C02 / 25Q128 / 内部Flash
 * @return {*}
 */
void ota_hardware_test(void)
{
    static uint8_t qbuf[256];   /* 写缓冲 */
    static uint8_t qrd[256];    /* 读回缓冲 */
    static uint32_t fdata[16];  /* 内部Flash写数据 */
    uint16_t i;
    uint8_t pass;
    uint32_t t;
    uint8_t sr;

    /* --- 1. 24C02 读写测试 (地址0x40起16字节, 避开OTA标志区0~47) --- */
    for (i = 0; i < 16; i++) qbuf[i] = (uint8_t)(0x50 + i);
    ota_24C02_WriteData(0x40, qbuf, 16);
    delay_ms(20);                               /* 等EEPROM写周期完成 */
    ota_24C02_ReadData(0x40, qrd, 16);
    pass = 1;
    for (i = 0; i < 16; i++) if (qbuf[i] != qrd[i]) pass = 0;
    printf("[24C02] %s\r\n", pass ? "PASS" : "FAIL");
    for (i = 0; i < 16; i++) printf("%02X ", qrd[i]);
    printf("\r\n");

    /* --- 2. 25Q128 测试: 擦除/页写带超时, 监控状态寄存器 --- */
    printf("[25Q128] ID=0x%04X\r\n", norflash_read_id());
    sr = norflash_read_sr(1);
    printf("[25Q128] SR1 before=0x%02X\r\n", sr);

    norflash_write_enable();                    /* 写使能 0x06 */
    sr = norflash_read_sr(1);
    printf("[25Q128] SR1 after WEN=0x%02X\r\n", sr);   /* WEL应=1 */

    NORFLASH_CS(0);
    spi1_read_write_byte(FLASH_SectorErase);    /* 0x20 扇区擦除 */
    spi1_read_write_byte(0xFF);                 /* 地址 0xFF0000 */
    spi1_read_write_byte(0x00);
    spi1_read_write_byte(0x00);
    NORFLASH_CS(1);

    for (t = 0; t < 100; t++)                   /* 最多等10秒 */
    {
        sr = norflash_read_sr(1);
        if (!(sr & 0x01)) break;
        delay_ms(100);
    }
    printf("[25Q128] SR1 after erase=0x%02X, %s\r\n", sr,
               (t < 100) ? "ERASE OK" : "TIMEOUT");

    if (t < 100)                                /* 擦除成功才继续 */
    {
        for (i = 0; i < 256; i++) qbuf[i] = i;

        norflash_write_enable();                /* 写使能 */
        NORFLASH_CS(0);
        spi1_read_write_byte(FLASH_PageProgram);/* 0x02 页编程 */
        spi1_read_write_byte(0xFF);             /* 地址 0xFF0000 */
        spi1_read_write_byte(0x00);
        spi1_read_write_byte(0x00);
        for (i = 0; i < 256; i++) spi1_read_write_byte(qbuf[i]);
        NORFLASH_CS(1);

        for (t = 0; t < 100; t++)               /* 最多等1秒 */
        {
            sr = norflash_read_sr(1);
            if (!(sr & 0x01)) break;
            delay_ms(10);
        }
        printf("[25Q128] SR1 after page=0x%02X, %s\r\n", sr,
                   (t < 100) ? "PAGE OK" : "TIMEOUT");

        ota_25q128_read(qrd, 0xFF0000, 256);
        pass = 1;
        for (i = 0; i < 256; i++) if (qbuf[i] != qrd[i]) pass = 0;
        printf("[25Q128] %s\r\n", pass ? "PASS" : "FAIL");
        for (i = 0; i < 16; i++) printf("%02X ", qrd[i]);
        printf("\r\n");
    }

    /* --- 3. 内部Flash 读写测试 (扇区3 @0x0800C000, 程序未占用) --- */
    for (i = 0; i < 16; i++) fdata[i] = 0x11220000 + i;
    __disable_irq();                            /* 擦写内部Flash期间关中断 */
    ota_flash_eraseflash(3, 1);                 /* 擦除扇区3(16KB) */
    ota_flash_writeflash(0x0800C000, fdata, 64);/* 写16个字 = 64字节 */
    __enable_irq();
    pass = 1;
    for (i = 0; i < 16; i++) if (((uint32_t *)0x0800C000)[i] != fdata[i]) pass = 0;
    printf("[FLASH] %s\r\n", pass ? "PASS" : "FAIL");
    for (i = 0; i < 16; i++) printf("%08X ", ((uint32_t *)0x0800C000)[i]);
    printf("\r\n");
}
