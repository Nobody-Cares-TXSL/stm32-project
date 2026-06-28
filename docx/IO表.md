# STM32F103 开发板 IO 分配表

> 来源：`IO表v0.0.docx`

## 板载资源

### 显示屏（SPI 接口）

| 编号 | 功能 | 模块标识 | 开发板引脚 | 备注 |
|------|------|----------|------------|------|
| 1 | 电源负极 | GND | GND | |
| 2 | 电源正极 | VCC | 3V3 | |
| 3 | SPI 时钟 | SCL | PB4 | |
| 4 | SPI 数据 | SDA | PB5 | |
| 5 | SPI 复位 | RES | PB6 | |
| 6 | SPI 数据/命令 | DC | PB7 | |
| 7 | SPI 片选 | CS | PB8 | |
| 8 | SPI 背光 | BLK | PB9 | |

### 按键与指示灯

| 编号 | 功能 | 模块标识 | 开发板引脚 | 备注 |
|------|------|----------|------------|------|
| 1 | 指示灯 D2 | PA8 | PA8 | 板载 LED |
| 2 | 按键 | WK-UP | PA0 | 按下高电平 |
| 3 | 按键 | K3 | PC8 | 按下低电平 |
| 4 | 按键 | K4 | PC9 | 按下低电平 |

### ESP8266 WIFI 模块（USART 接口）

| 编号 | 功能 | 模块标识 | 开发板引脚 | 备注 |
|------|------|----------|------------|------|
| 1 | 电源负极 | GND | GND | |
| 2 | 电源正极 | 3V3 | 3V3 | |
| 3 | USART | RX | PA2 | |
| 4 | USART | TX | PA3 | |

### 摇杆模块

| 编号 | 功能 | 模块标识 | 开发板引脚 | 备注 |
|------|------|----------|------------|------|
| 1 | X 轴 | ADC | PB1 | ADC12_IN9 |
| 2 | Y 轴 | ADC | PC4 | ADC12_IN14 |
| 3 | SW 按键 | GPIO | PC5 | 上拉输入，按下为低 |

## 硬件通信模块

| 编号 | 功能 | 模块标识 | 开发板引脚 | 备注 |
|------|------|----------|------------|------|
| 1 | ISP 串口发送 | ISP | PA9 | |
| 2 | ISP 串口接收 | ISP | PA10 | |
| 3 | J-Link 数据接口 | DIO | PA13 | SWDIO |
| 4 | J-Link 时钟接口 | CLK | PA14 | SWCLK |
| 5 | 低速外部时钟输入 | OSC32IN | PC14 | 32.768kHz RTC |
| 6 | 低速外部时钟输出 | OSC32OUT | PC15 | |
| 7 | 启动方式选择 | BOOT1 | PB2 | 与 BOOT0 配合 |

## STM32F103RCT6 引脚功能表

| 引脚编号 | 引脚名称 | 默认功能 | 主要复用功能 |
|----------|----------|----------|--------------|
| 1 | PB2/BOOT1 | GPIO (B2) / BOOT1 | - |
| 2 | PB10 | GPIO (B10) I/O | I2C2_SCL / USART3_TX |
| 3 | PB11 | GPIO (B11) I/O | I2C2_SDA / USART3_RX |
| 4 | PB12 | GPIO (B12) I/O | I2C2_SMBAl / SPI2_NSS / USART3_CK |
| 5 | PB13 | GPIO (B13) I/O | SPI2_SCK / USART3_CTS |
| 6 | PB14 | GPIO (B14) I/O | SPI2_MISO / USART3_RTS / TIM1_CH2N |
| 7 | PB15 | GPIO (B15) I/O | SPI2_MOSI / TIM1_CH3N |
| 8 | PC13 | GPIO (C13) I/O | TAMPER-RTC |
| 9 | PC14 | GPIO (C14) I/O | OSC32_IN |
| 10 | PC15 | GPIO (C15) I/O | OSC32_OUT |
| 11 | PD0 | GPIO (D0) I/O | FSMC_D2 |
| 12 | PD1 | GPIO (D1) I/O | FSMC_D3 |
| 13 | NRST | 复位 (I) | - |
| 14 | PC0 | GPIO (C0) I/O | ADC12_IN10 |
| 15 | PC1 | GPIO (C1) I/O | ADC12_IN11 |
| 16 | PC2 | GPIO (C2) I/O | ADC12_IN12 |
| 17 | PC3 | GPIO (C3) I/O | ADC12_IN13 |
| 18 | VSSA | 模拟地 (P) | - |
| 19 | VDDA | 模拟电源 (S) | - |
| 20 | PA0-WKUP | GPIO (A0) I/O | ADC12_IN0 / TIM2_CH1_ETR / USART2_CTS / WKUP |
| 21 | PA1 | GPIO (A1) I/O | ADC12_IN1 / TIM2_CH2 / USART2_RTS |
| 22 | PA2 | GPIO (A2) I/O | ADC12_IN2 / TIM2_CH3 / USART2_TX |
| 23 | PA3 | GPIO (A3) I/O | ADC12_IN3 / TIM2_CH4 / USART2_RX |
| 24 | VSS | 数字地 (P) | - |
| 25 | VDD | 数字电源 (S) | - |
| 26 | PA4 | GPIO (A4) I/O | ADC12_IN4 / SPI1_NSS / USART2_CK |
| 27 | PA5 | GPIO (A5) I/O | ADC12_IN5 / SPI1_SCK |
| 28 | PA6 | GPIO (A6) I/O | ADC12_IN6 / SPI1_MISO / TIM1_BKIN |
| 29 | PA7 | GPIO (A7) I/O | ADC12_IN7 / SPI1_MOSI / TIM1_CH1N |
| 30 | PC4 | GPIO (C4) I/O | ADC12_IN14 |
| 31 | PC5 | GPIO (C5) I/O | ADC12_IN15 |
| 32 | PB0 | GPIO (B0) I/O | ADC12_IN8 / TIM1_CH2N |
| 33 | PB1 | GPIO (B1) I/O | ADC12_IN9 / TIM1_CH3N |
| 34 | PB3 | GPIO (B3) I/O | SPI1_SCK / TIM2_CH2 / TRACESWO |
| 35 | PB4 | GPIO (B4) I/O | SPI1_MISO / TIM3_CH1 |
| 36 | PB5 | GPIO (B5) I/O | SPI1_MOSI / TIM3_CH2 / I2C1_SMBA |
| 37 | PB6 | GPIO (B6) I/O | I2C1_SCL / TIM4_CH1 / USART1_TX |
| 38 | PB7 | GPIO (B7) I/O | I2C1_SDA / TIM4_CH2 / USART1_RX |
| 39 | BOOT0 | 启动模式 (I) | - |
| 40 | PB8 | GPIO (B8) I/O | I2C1_SCL / TIM4_CH3 / SDIO_D4 |
| 41 | PB9 | GPIO (B9) I/O | I2C1_SDA / TIM4_CH4 / SDIO_D5 |
| 42 | VSS | 数字地 (P) | - |
| 43 | VDD | 数字电源 (S) | - |
| 44 | PE0 | GPIO (E0) I/O | TIM4_ETR |
| 45 | PE1 | GPIO (E1) I/O | - |
| 46 | PE2 | GPIO (E2) I/O | - |
| 47 | PE3 | GPIO (E3) I/O | - |
| 48 | PE4 | GPIO (E4) I/O | - |
| 49 | PE5 | GPIO (E5) I/O | - |
| 50 | PE6 | GPIO (E6) I/O | - |
| 51 | PC6 | GPIO (C6) I/O | - |
| 52 | PC7 | GPIO (C7) I/O | - |
| 53 | PC8 | GPIO (C8) I/O | SDIO_D0 / TIM8_CH1 |
| 54 | PC9 | GPIO (C9) I/O | SDIO_D1 / TIM8_CH2 / I2C3_SDA |
| 55 | PA8 | GPIO (A8) I/O | USART1_CK / TIM1_CH1 / I2C2_SCL |
| 56 | PA9 | GPIO (A9) I/O | USART1_TX / TIM1_CH2 / I2C2_SMBA |
| 57 | PA10 | GPIO (A10) I/O | USART1_RX / TIM1_CH3 |
| 58 | PA11 | GPIO (A11) I/O | USART1_CTS / CANRX / TIM1_CH4 |
| 59 | PA12 | GPIO (A12) I/O | USART1_RTS / CANTX / TIM1_ETR |
| 60 | PA13 | GPIO (A13) I/O | JTMS-SWDIO |
| 61 | VSS | 数字地 (P) | - |
| 62 | VDD | 数字电源 (S) | - |
| 63 | PA14 | GPIO (A14) I/O | JTCK-SWCLK |
| 64 | PA15 | GPIO (A15) I/O | JTDI / SPI1_NSS |

## 关键引脚补充说明

- **OSC_IN (PC14) / OSC_OUT (PC15)**：连接外部低速 32.768kHz 晶振，用于 RTC。
- **OSC_IN (PH0) / OSC_OUT (PH1)**：LQFP-64 封装中 PH0/PH1 分别对应 PC14/PC15，连接外部高速 4-16MHz 晶振。
- **NRST**：外部复位引脚，低电平有效。
- **VBAT**：LQFP-64 封装中与 PC13 共用，为 RTC 和备份寄存器供电。
- **BOOT0 和 PB2/BOOT1**：电平组合决定芯片启动模式（主 Flash / 系统存储器 ISP / SRAM）。
- **PA13 (SWDIO) 和 PA14 (SWCLK)**：SWD 串行调试接口，用于程序下载和调试。
- **VDD / VSS**：所有 VDD 必须供电，所有 VSS 必须接地，每个 VDD 与最近 VSS 之间建议接 100nF 退耦电容。
- **VDDA / VSSA**：模拟电源和地，即使不使用 ADC/DAC 也建议连接，通过磁珠或电感与数字电源隔离。
