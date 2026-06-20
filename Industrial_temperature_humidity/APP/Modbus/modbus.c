#include "modbus.h"

QueueHandle_t lvgl_data_queue = NULL;   /* 给LVGL的温湿度队列 */

/**
 * @description:  初始化RS485+Modbus: 建队列 + rs485_init(9600)
 * @return {*}
 */
void Modbus_Init(void)
{
    if(lvgl_data_queue != NULL) return;     // 队列已创建

    lvgl_data_queue = xQueueCreate(1, sizeof(Modbus_TH_t));

    rs485_init(9600);                           /* USART2 9600-8N1, 方向脚默认接收 */
}



/**
 * @description: 
 * @param {uint8_t} slave_addr 从机地址
 * @param {uint8_t} command_code 功能码
 * @param {uint16_t} Start_addr 起始地址
 * @param {uint16_t} Register_count 寄存器数量
 * @param {uint16_t} *rbuf 接收缓存
 * @return {*} 0失败 1成功
 */
uint8_t Modbus_communication_Data(uint8_t slave_addr,uint8_t command_code,uint16_t Start_addr,uint16_t Register_count,uint16_t *rbuf)
{
    uint8_t i=0;
    uint8_t tx_data[8]={0};
    uint8_t rx_data[32];
    uint8_t rx_count=0;
    uint16_t crc=0;

    /* 1. 构造读取的报文格式
        从机地址(设备编号)(1 byte)+功能码(1 byte)+寄存器地址(2 byte)+寄存器数量(2 byte)校验(2 byte) = 8byte
    */
    tx_data[0] = slave_addr;                        // 从机地址
    tx_data[1] = command_code;                      // 功能码
    tx_data[2] = (Start_addr >> 8) & 0xFF;          // 寄存器地址高四位
    tx_data[3] = Start_addr & 0xFF;                 // 寄存器地址低四位
    tx_data[4] = (Register_count >> 8) & 0xFF;      // 寄存器数量高四位
    tx_data[5] = Register_count & 0xFF;             // 寄存器数量低四位
    crc=Modbus_CRC16(tx_data,6);                   // 计算前面CRC16校验码
    tx_data[6]=crc&0xFF;                            //CRC16校验码低八位
    tx_data[7]=(crc>>8)&0xFF;                       //CRC16校验码高八位


    memset(rx_data,0,sizeof(rx_data));
    /* 2. 清空残留数据,下一轮从"干净状态"开始*/
    __HAL_UART_CLEAR_FLAG(&rs458_handler, UART_FLAG_RXNE | UART_FLAG_ORE);

    /* 3. 发送数据 */
    RS485_RE(1);    // 切换为发送模式
    HAL_StatusTypeDef tx_status = HAL_UART_Transmit(&rs458_handler,tx_data,8,1000);
    if(tx_status != HAL_OK) printf("[MODBUS] TX err=%d\r\n", (int)tx_status);
    vTaskDelay(1);
    RS485_RE(0);    // 切换为接收模式

    /* 4.中断接收数据: ISR收进 RS485_rx_buf, rs485_receive_data() 以10ms帧间隙判稳定后取整帧 */
    uint8_t rl = 0; //判断是否接收到数据
    for(uint8_t i = 0; i < 5; i++)
    {
        rs485_receive_data(rx_data, &rl);
        if(rl) break;   //确定接收到数据
        vTaskDelay(2);  //2ms 轮询一次 如果确认没收到后, 让CPU休息2ms再问一次
    }
    rx_count = (rl > sizeof(rx_data)) ? sizeof(rx_data) : rl;    //防止数组越界

    /* 5. 解析数据 按照我的传感器要求
        从机地址(设备编号)(1 byte)+功能码(1 byte)+字节数量(1 byte)+寄存器值(2 byte)+校验(2 byte) = 8byte
    */
    uint8_t data_len = rx_data[2];           /* 取出字节数量 */
    uint16_t crc_Re_value = Modbus_CRC16(rx_data,rx_count-2);

    /* 比对数据是否有效 比对CRC16校验码和功能码和从机地址和字节数量*/
    if((rx_count == (3 + data_len + 2)) && (rx_data[0] == slave_addr) && (rx_data[1] == command_code)
        && (rx_data[rx_count-2] == (crc_Re_value & 0xFF)) && (rx_data[rx_count-1] == ((crc_Re_value >> 8)&0xFF)))
    {
        /*数据转存到buf*/
        for(i=0 ;i < Register_count;i++)
        {
            rbuf[i] = (rx_data[3+i*2]<<8)|rx_data[4+i*2];
        }
        return 1;
    }
    return 0;
}

/* 结构体数组 多从机配置: 从机地址, 功能码, 起始寄存器, 寄存器数量, 错误计数, 数据 */
Modbus_t dev_list[]={
    {0x01,0x03,0x0001,2,0,0,0},  /* 1# 温湿度 */
    //如果多个从机 可以继续添加 
};
// 从机数量
uint8_t dev_cnt = sizeof(dev_list)/sizeof(Modbus_t);

/**
 * @description: 获取所有从机数据
 * @return {*}
 */
void Modbus_Receive_all_data(void)
{
    uint16_t tmp_buf[8];
    for(uint8_t i=0;i<dev_cnt;i++)      /* dev_cnt数组元素个数 */
    {
        Modbus_t *p = &dev_list[i];
        memset(tmp_buf, 0, sizeof(tmp_buf));


        
        /* 发起读取 */
        switch(p->slave_addr)
        {
            case 0x01: 
                if(Modbus_communication_Data(p->slave_addr,p->command_code,p->reg_start,1,tmp_buf)==1)
                {
                    p->temp = tmp_buf[0]/10.0f;          /* 温度 固定1位小数 */
                }
                if(Modbus_communication_Data(p->slave_addr,p->command_code,p->reg_start+1,1,tmp_buf)==1)
                {
                    p->humi = tmp_buf[0]/10.0f;          /* 湿度 固定1位小数 */
                }
                break;
            //如果多个从机 可以继续添加case
            default:break;
        }
        vTaskDelay(pdMS_TO_TICKS(30)); /* 从机间隔防总线干扰 */
    }
}


/**
 * @description: 上传温湿度到LVGL队列(覆盖写)
 * @return {*}
 */
void Modbus_Updata_to_LVGL(void)
{
    Modbus_TH_t th;
    th.temp = dev_list[0].temp;
    th.humi = dev_list[0].humi;
    if(lvgl_data_queue != NULL)
    {
        xQueueOverwrite(lvgl_data_queue, &th);
    }
}

/**
 * @description: CRC校验
 * @param {uint8_t} *data
 * @param {uint8_t} len
 * @return {*}
 */
uint16_t Modbus_CRC16(uint8_t *data, uint16_t datalen)
{
    uint8_t i;
    uint16_t Crcinit  = 0xffff;
    uint16_t Crcipoly = 0xA001;          /* 多项式必须换成反射形式 */
    while(datalen--)
    {
        Crcinit ^= *data++;              /* 数据从低8位进 */
        for(i=0;i<8;i++)
        {
            if(Crcinit & 1)              /* 判最低位 */
                Crcinit = (Crcinit >> 1) ^ Crcipoly;   /* 右移 */
            else
                Crcinit >>= 1;
        }
    }
    return Crcinit;
}
