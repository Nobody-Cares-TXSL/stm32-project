# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 项目概况

STM32F10x 嵌入式 C 项目，使用 **STM32 标准外设库**（非 HAL/LL），C17 标准，目标芯片 STM32F10X_MD（中容量）。

## 构建环境

- **标准外设库路径**：`~/stm32_std_lib/`（含 CMSIS、StdPeriph_Driver、Template）
- **IDE 配置**：`.vscode/c_cpp_properties.json` 已配置 IntelliSense 路径

## 架构

**Superloop + 菜单调度**模式。`main.c` 为入口，初始化后进入主循环，每轮通过摇杆事件驱动菜单或子模块。

```
main.c                      入口 + 主循环（调用 menu_handle）
code/
  menu.c / menu.h            菜单框架：光标移动 + 子模块调度
  xxx_view.c / xxx_view.h    功能页面（enter/tick/on_event 三接口）
  joystick.c / joystick.h    摇杆 ADC + SW 按键
  dht.c / dht.h              DHT11 温湿度驱动
  smoke.c / smoke.h          MQ-2 烟雾传感器驱动
  smoke_view.c / smoke_view.h  
  fan.c / fan.h              PB0 TIM3_CH3 PWM 
  fan_view.c / fan_view.h    风扇控制视图
  light.c / light.h          PC3(AO ADC13) + PB13(DO) light sensor
  light_view.c / light_view.h    光敏传感器视图
  flame.c / flame.h          PC1/PC2(AO) + PB11/PB12(DO) flame x2
  flame_view.c / flame_view.h
  pump.c / pump.h            PA1 GPIO -> relay -> water pump
  pump_view.c / pump_view.h
  ledstrip.c / ledstrip.h  PC11+PC12 DIN WS2812B
  ledstrip_view.c / ledstrip_view.h
  lcd.c / lcd.h              LCD 底层驱动
  lcd_ui.c / lcd_ui.h        LCD 上层封装（登录画面等）
  lcdfont.h                  16px 中文字模（GB2312）
  io.c / io.h                GPIO/EXTI/NVIC/ADC/TIM 通用初始化
  delay.c / delay.h          延时函数
  sys.h                      系统级宏
```

### 菜单与子模块

- `menu_items[]` 数组定义所有菜单项，每项包含 text + enter/tick/on_event 三个回调
- `menu_handle(ev, sw)` 统一处理：主菜单态移动光标，子模块态调用 tick/on_event
- 子模块 `on_event` 返回 1 → 退回主菜单
- 新增功能：写 `xxx_view.c/h`，在 `menu_items[]` 添加一项即可

## 代码风格

- **函数命名**：`snake_case`，模块前缀 `MODULE_action()`（如 `GPIO_init`、`joystick_event`、`th_view_enter`）
- **宏/常量**：`UPPER_CASE`（`LED_PIN`、`JOY_MID`、`FLASH_TIME`）
- **硬件动作**：大写无参宏封装（`LED_ON`、`BEEP_OFF`），低电平有效/高电平有效见 `io.h` 注释
- **变量**：`snake_case`，局部变量简短（`ch`、`t`、`h`），全局变量描述性命名
- **ISR 共享变量**：必须 `volatile`
- **文件内部函数**：`static` 限制作用域
- **头文件守卫**：`#ifndef __MODULE_H` 双下划线风格
- **注释语言**：中文，`/* ====== 分段标题 ====== */` 标记逻辑区块

## 文件编码

- `code/` 下全部源文件统一 **GB2312** 编码（Keil 按 GBK 解析中文）
- 新建文件须转 GB2312（PostToolUse hook 会自动检测提醒）

## 关键约定

- **串口**：USART1，9600 baud，接收缓冲 `USART1_RX_BUF[100]`
- **接收处理模式**：效果函数每次处理完**必须** `memset(USART1_RX_BUF, 0, sizeof(...))` + `USART1_RX_STA = 0`，否则主循环重复进入
- **摇杆 ADC**：PB1(X)/PC4(Y)，SW(PA2，板子设计为 USART2_TX，当前暂借给摇杆)，死区 1500-2500，边沿触发
- **DHT11**：PB14（传感器1）、PB15（传感器2）
- **烟雾 MQ-2**：PC0(AO, ADC 通道 10)、PB10(DO)
- **风扇**：PB0(TIM3_CH3 PWM)
- **光敏**：PC3(AO, ADC通道 13)、PB13(DO)
- **火焰**：PC1+PC2(AO, ADC通道 11/12)、PB11+PB12(DO)
- **水泵**：PA1(GPIO -> relay IN1)
- **灯带**：PC12(DIN1) + PC11(DIN2) WS2812B
- **LCD**：128x128，SPI 驱动，16px 字号，`lcdfont.h` 含 GB2312 中文字模
- **引脚定义**：全部在 `io.h` 用宏集中定义，硬件连接变动只改此处
- **模块新增**：新 `.c/.h` 文件放入 `code/`，头文件包含 `stm32f10x.h`，`xxx_init()` 在 `main.c` 的 `joystick_init()` 后调用（确保 ADC1 已初始化）
