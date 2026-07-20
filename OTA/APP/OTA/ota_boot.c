#include "ota_boot.h"

LOAD_APP Load_App; //利用函数指针获取应用程序入口地址


/**
 * @description: bootloader跳转到应用程序
 * @return {*}
 */
void ota_boot_Jump(void)
{
    if(ota_boot_Enter(2) == 0) //    printf("ds内,输入小写字母w,进入Bootloader命令行\r\n",timeout); 多少就是多少秒
    {
        if(ota_flag.OTA_up_Flag == OTA_FLAG)        //向内部flash中下载程序
        {
            printf("ota开始升级\r\n");             
            Boot_State_flag |= OTA_updata_APP_Flag; //OTA_updata_APP_Flag置位,APP更新程序
            UpDataAPP.NM25Q128_BlockNumber = 0;
        }
        else
        {
            printf("ota跳转A区\r\n");
            ota_Load_app(OTA_APP_Start_Addr);  /* 跳转A区 (内部先清理外设) */
        }
    }
    else // 进入命令行模式
    {
        printf("进入命令行模式\r\n");
        ota_boot_information();
    }
}


/**
 * @description: 进入bootloader命令行模式
 * @param {uint8_t} timeout 单位秒
 * @return {*}
 */
uint8_t ota_boot_Enter(uint8_t timeout)
{
    printf("%ds内,输入小写字母w,进入Bootloader命令行\r\n",timeout);  
	
    while(timeout--)
    {
        delay_ms(1000);
        if(ota_uasrt1_rx_buf[0] == 'w')
        {
            return 1;   // 进入命令模式 
        }

    }
    return 0; // 没有进入命令模式 
}




void ota_boot_information(void)
{
    printf("\r\n\r\n");
    printf("=========[1]擦除APP============\r\n");
    printf("=========[2]软件重启===========\r\n");
    printf("=========[3]设置版本号=========\r\n");
    printf("=========[4]查询版本号=========\r\n");
}

void ota_boot_Event(uint8_t *data,uint16_t datalen)
{
    int temp;

    if(Boot_State_flag == 0)
    {
       if((datalen == 1) && data[0] == '1')
        {
            printf("擦除APP\r\n");
            ota_flash_eraseflash(OTA_APP_Sector,OTA_APP_SECTOR_NUM);    //擦除APP区 OTA_APP_SECTOR_NUM按需求改
        }
        else if ((datalen == 1) && data[0] == '2')
        {
            printf("软件重启\r\n");
            delay_ms(100);
            NVIC_SystemReset();
        } 
        else if ((datalen == 1) && data[0] == '3')
        {
            printf("设置OTA版本号\r\n");
            Boot_State_flag |= SET_VERSION_Flag;
        }
        else if ((datalen == 1) && data[0] == '4')
        {
            printf("查询OTA版本号\r\n");
            ota_24C02_ReadOTAFlag();
            printf("OTA版本号:%s",ota_flag.OTA_Version);
            ota_boot_information();
        }
    }
    else if(Boot_State_flag & SET_VERSION_Flag)
    {
        if (datalen == 26)
        {
            if(sscanf((char *)data,"VER-%d.%d.%d-%d/%d/%d-%d:%d",&temp,&temp,&temp,&temp,&temp,&temp,&temp,&temp)==8)
            {
                memset(ota_flag.OTA_Version,0,32);
                memcpy(ota_flag.OTA_Version,data,26);
                ota_24C02_WriteOTAFlag();   //写到对应结构体中并保存到24c02
                printf("版本号正确\r\n");
                Boot_State_flag &= ~SET_VERSION_Flag;
                ota_boot_information();

            }
            else
            {
                printf("版本号格式错误\r\n");
            }
        }
        else
        {
            printf("版本号长度错误\r\n");
        }
    }
    
}

/**
 * @description: 设置栈顶地址
 * @param {uint32_t} addr
 * @return {*}
 */
void set_msp(uint32_t addr)
{
	__set_MSP(addr); /* 将addr写入MSP */
}

/**
 * @description: PC指针 地址为SP+4
 * @param {uint32_t} addr:OTA_APP_Start_Addr
 * @return {*}
 */
void ota_Load_app(uint32_t addr)
{
    if((*(uint32_t*)addr >= 0x20000000) && (*(uint32_t*)addr <= 0x2002FFFF)) //判断地址是否合法
    {
        ota_boot_clear();              /* 跳转前: 复位外设+时钟, 关中断(APP侧自行恢复中断) */
        set_msp(*(uint32_t*)addr);     //设置栈顶地址
        Load_App = (LOAD_APP)(*(uint32_t*)(addr + 4)); //Load_App指向复位向量地址
        Load_App();      //调用使得复位向量地址给到PC，跳转到应用程序
    }
    else
    {
        printf("ota跳转A区失败\r\n");
        printf("进入命令行模式\r\n");
        ota_boot_information();
    }
}

void ota_boot_clear(void)
{
    __disable_irq();                   /* 1. 关全局中断；记得在APP区的时候开启；否则会出现其APP区中断程序运行不了这个情况 */
    HAL_DeInit();                      /* 2. 全外设+时钟复位到上电状态 */
    SysTick->CTRL = 0;                 /* 3. 关SysTick */
}


