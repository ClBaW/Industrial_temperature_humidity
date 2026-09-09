<!--
 * @Author: ClBaW
 * @Date: 2026-09-09 15:20:56
 * @LastEditTime: 2026-09-09 15:38:25
-->
# 工业温湿度监测系统（STM32F407 + OTA）

基于 STM32F407ZGT6 + FreeRTOS + LVGL 开发的工业级温湿度监测终端，通过 Modbus-RTU 总线采集温湿度数据，本地 LVGL 触摸屏实时显示，并支持基于 ESP8266 Wi-Fi 的 OTA 固件远程升级。适配工业现场 7×24 小时无人值守运行场景。

## 系统架构

```
┌──────────────────────────────────────────────────────┐
│                  OTA 升级服务器 (TCP)                  │
│                 （IP/端口与升级包由用户部署）           │
└──────────────────────────────────────────────────────┘
                              ▲
                              │ Wi-Fi (ESP8266 AT 透传)
                              ▼
┌──────────────────────────────────────────────────────┐
│              STM32F407ZGT6 主控 (FreeRTOS)            │
│  ┌────────────────────────────────────────────────┐  │
│  │ 2.8吋触摸屏 (LVGL)   首页/设置/告警 三页交互      │  │
│  └────────────────────────────────────────────────┘  │
│  ┌────────────┐ ┌────────────┐ ┌──────────────────┐  │
│  │ Modbus-RTU │ │ W25Q128    │ │ 24C02 EEPROM     │  │
│  │ 多从机采集 │ │ OTA固件区  │ │ WiFi/服务器/参数   │  │
│  └────────────┘ └────────────┘ └──────────────────┘  │
│         │                                            │
│         │ RS485 (TP485E)                             │
└─────────┼────────────────────────────────────────────┘
          ▼
    温湿度传感器等 Modbus 从机
```

## 硬件清单

| 物料 | 型号 | 用途 |
|---|---|---|
| 主控 | STM32F407ZGT6 | 主控制器，运行业务逻辑与界面 |
lvgl
| 显示屏 | 3.5寸触摸屏（ST7789） 使用FSMC并口| LVGL 图形交互、数据展示 |
| SRAM | 1MB | 如果显示缓冲区放在内部就可以不需要 |
wifi
| Wi-Fi 模块 | 拼夕夕ESP8266或者正点原子的也行（AT 固件） | 联网、OTA 固件下载 |
| RS485 收发器 | TP485E | 电平转换，Modbus 总线通信 |
| 温度传感器 | 拼夕夕8块钱左右 | 程序下载与在线调试 |
ota
| 外置 Flash | NW25Q128 | OTA 固件存储 |
| EEPROM | AT24C02 | 参数与配置存储 |
| 调试器 | ST-Link  | 程序下载与在线调试 |

具体引脚分配见相关初始化配置即可

## 硬件屏幕更改->软件输出输入更改 
更改 \Middlewares\LVGL\lvgl\examples\portin\lv_port_disp_template_ClBaW和lv_port_indev_template_ClBaW
lv_port_disp_template_ClBaW     输出 也就是屏幕配置
lv_port_indev_template_ClBaW    输入 也就是触摸屏配置

## 软件架构

FreeRTOS 任务划分（`User/RTOS_TASK.c`）：

| 任务 | 职责 |
|---|---|
| start_task | 系统启动与各任务创建 |
| lv_demo_task | LVGL 界面刷新、页面切换、触摸交互 |
| modbus_task | Modbus 多从机轮询采集 |
| ota_wifi_task | ESP8266 联网、固件下载状态机 |

任务间通过消息队列传递采集数据与界面刷新指令，串口/Flash 等共享资源使用互斥锁保护。

## 核心功能

1. **Modbus-RTU 采集**：多从机轮询温湿度传感器，CRC16 校验，可扩展设备。
2. **LVGL 图形界面**：三页交互（首页数据总览、告警状态、参数设置），自定义中文字体（16/36/48px），控件按需刷新。
3. **OTA 远程升级**：ESP8266 联网后连接升级服务器，接收固件写入 W25Q128，校验完成后跳转升级；配合独立的 Bootloader（`OTA/` 工程）实现断点续升级与失败回滚。 可实现断电和断网不变转
4. **参数持久化**：WiFi 名称/密码、服务器地址、报警上下限等存入 24C02 EEPROM，掉电不丢失。
5. **报警功能**：温湿度超上限/下限告警，设备在线/离线状态检测。

## 快速使用

1. 编译烧录：先烧录 Bootloader（`OTA/` 工程），再烧录主程序（`Industrial_temperature_humidity/` 工程）；
2. 配置网络：修改 `APP/OTA_wifi/ota_wifi.h` 中的 WiFi SSID/密码、升级服务器 IP/端口；
3. 升级包生成：Keil 编译主工程后自动调用 `Tools/make_ota_pkg.py` 生成 `bin/Industrial_temperature_humidity_ota.bin`，上传至升级服务器；
4. 运行验证：上电后通过界面查看温湿度数据，网络正常时可触发固件升级流程。

## 项目结构

```
Industrial_temperature_humidity/
├── Industrial_temperature_humidity/   主程序工程（APP 固件）
│   ├── APP/          Modbus、OTA_WiFi、UI 业务代码
│   ├── User/         main、RTOS 任务
│   ├── Drivers/      BSP 板级驱动
│   └── Middlewares/  FreeRTOS、LVGL、FATFS
├── OTA/              Bootloader 工程（OTA 升级引导）
└── Tools/            升级包生成等辅助脚本
```

## 技术栈

STM32F4 HAL 库 · FreeRTOS · LVGL 9 · Modbus-RTU · ESP8266 AT · OTA · W25Q128 · 24C02 · CRC16
