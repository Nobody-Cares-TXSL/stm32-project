# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## 项目概况

STM32F10x 嵌入式 C 项目，使用 **STM32 标准外设库**（非 HAL/LL），C17 标准，目标芯片 STM32F10X_MD（中容量）。

## 构建环境

- **标准外设库路径**：`~/stm32_std_lib/`（含 CMSIS、StdPeriph_Driver、Template）
- **IDE 配置**：`.vscode/c_cpp_properties.json` 已配置 IntelliSense 路径

## 架构

**Superloop + 中断驱动**模式。`main.c` 为中央调度器，主循环轮询串口/摇杆事件并按 `effect_index` 分发到对应效果函数。

```
main.c                  入口 + 主循环调度
code/
  
```

## 代码风格

- **函数命名**：`snake_case`，模块前缀 `MODULE_action()`（如 `GPIO_init`、`USART1_Send_Str`、`joystick_event`）
- **宏/常量**：`UPPER_CASE`（`LED_PIN`、`JOY_MID_MIN`、`FLASH_TIME`）
- **硬件动作**：大写无参宏封装（`LED_ON`、`BEEP_OFF`），低电平有效/高电平有效见 `io.h` 注释
- **变量**：`snake_case`，局部变量简短（`ch`、`nus`、`ms`），全局变量描述性命名
- **ISR 共享变量**：必须 `volatile`
- **文件内部函数**：`static` 限制作用域（如 `joystick_dir`、`member_lookup`）
- **头文件守卫**：`#ifndef __MODULE_H` 双下划线风格
- **注释语言**：中文，`/* ====== 分段标题 ====== */` 标记逻辑区块

## 状态管理


## 关键约定

- **串口**：USART1，9600 baud，接收缓冲 `USART1_RX_BUF[100]`
- **接收处理模式**：效果函数每次处理完**必须** `memset(USART1_RX_BUF, 0, sizeof(...))` + `USART1_RX_STA = 0`，否则主循环重复进入
- **摇杆 ADC**：PC1(X)/PC2(Y)，死区 1500-2500，边沿触发（中点→方向才返回事件）
- **引脚定义**：全部在 `io.h` 用宏集中定义，硬件连接变动只改此处
- **模块新增**：新 `.c/.h` 文件放入 `code/`，头文件包含 `stm32f10x.h`，在 `main.c` 顶部 `#include`
