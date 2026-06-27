---
name: add-peripheral-module
description: Use when adding a new peripheral module (sensor/actuator: 温湿度/烟雾/风扇/光敏/火焰/继电器…) to this STM32F10x project that needs data collection, LCD display, and a menu entry. Also use when LCD shows garbled/duplicate Chinese chars, or when editing any GB2312 source file.
---

# 添加外设模块

## Overview

本项目（STM32F10x 标准外设库 + Superloop）按**固定范式**接入外设模块：

```
驱动层 xxx.c/.h   →   视图层 xxx_view.c/.h   →   菜单挂载（menu_items[] 加一行）
```

`menu.c` 的导航框架（`active_view` 分发）对所有模块通用——加新模块**只挂回调、不改框架**。

已验证模块：温湿度（DHT11 单总线 ×2）、烟雾报警器（AO+DO）、风扇（PWM 调速）。照此复制即可。

## When to Use

- 用户要加新传感器/执行器，要求采集数据能显示到 LCD、菜单进入查看
- **编辑任何含中文的源文件时**（编码陷阱，见 GB2312 工作流）
- LCD 显示乱码/重复汉字（见 Common Mistakes 的两个高频坑）

不适用：纯改菜单文字、改既有模块逻辑、无硬件涉及的 UI 调整。

## 接入流程

### 1. 确认硬件（AskUserQuestion，必做）
无法从代码推断的事实先问清：
- **信号类型**：数字（DO/GPIO）/ 模拟（AO/ADC）/ 单总线（DHT11）/ PWM（执行器）
- **引脚**（AO 需 ADC 通道脚；DO 任意 GPIO；PWM 需 TIM 通道脚）
- **电平逻辑**（DO 低/高电平=触发；PWM 极性）→ 做成可配置宏，烧录后验证。**常见：蜂鸣器模块多为低电平有效（LOW=响）**

### 2. 检查引脚冲突
已占用：LCD(PB4-9)、摇杆(PB1/PC4/PA2)、温湿度(PB14/15)、烟雾(PC0/PB10)、风扇(PB0/TIM3_CH3)。
空闲 ADC 通道脚：PA0/PA1/PC1-PC3/PC5 等。
空闲 PWM 通道：TIM2_CH1(PA6)/CH2(PA7)/CH3(PB0-已占)/CH4(PB1-ADC)、TIM3_CH1(PA6)/CH2(PA7)等。

### 3. 检查字库（`code/inc/lcdfont.h` 的 `tfont16`）
显示文案必须用字库已有的字。一行提取清单：
```bash
python3 -c "import re;t=open('code/inc/lcdfont.h','rb').read().decode('gbk',errors='replace');m=re.search(r'tfont16\s*\[\]',t);s=t[m.end():];print(''.join(re.findall(r'/\*\"(.)\"',s[:re.search(r'tfont24',s).start()])))"
```
缺字时优先用现有字 + ASCII 规避（如"浓度"缺 → 用"烟雾"+数字）；实在要新字再问用户加字模。

### 4. 写驱动（`code/src/xxx.c` + `code/inc/xxx.h`，新建 GB2312）
- 引脚宏集中在文件顶部（换线只改宏）
- 复用 `io.c` 的 `GPIO_init(pin_type, pin, mode, GPIOx)`（每次自动使能时钟）；`pin_type` 用 `RCC_APB2Periph_TYPE_A/B/C`；`GPIO_Mode_AF_PP` 用于 PWM 输出引脚
- **PWM 模块**：复用 `io.c` 的 `TIM_base_init(TIMx, psc, arr)` + `TIM_pwm_init(TIMx, channel, pulse, polarity_high)`（调用前自行 `RCC_APB1PeriphClockCmd` 使能 TIM 时钟）。`fan.c` 是参考实现
- **ADC 复用 ADC1**（`joystick_init` 已初始化为单次/软件触发/右对齐/55.5 周期），自带 `static xxx_read_adc(u8 ch)`（照抄 `joystick.c`，~5 行）。单模块重复可接受；第三个 ADC 模块再抽公共 `adc.c`
- 单总线传感器（DHT11）：方向切换 `GPIO_Mode_Out_PP`/`GPIO_Mode_IPU`，所有等待**带超时计数**防卡死
- API 极简：`xxx_init()` + 控制函数

### 5. 写视图（`code/src/xxx_view.c` + `code/inc/xxx_view.h`，新建 GB2312，三件套）
标准接口（与 `th_view`/`smoke_view`/`fan_view` 同构）：
```c
void xxx_view_enter(void);             /* 进入：标题+提示+占位 */
void xxx_view_tick(void);              /* 主循环周期调用：节流重读+刷新数值（执行器可空操作） */
u8   xxx_view_on_event(u8 ev, u8 sw);  /* DOWN 返回 1(=退回主菜单) */
```
- 刷新节流：静态计数器累计到阈值才读传感器（ADC 类 ~0.5s/50 ticks；DHT 类 ~1s/100 ticks，因读取阻塞）
- **执行器模块**：tick 可空操作（状态仅随用户输入变化），on_event 处理 SW（开关）、L/R（调速）等操作
- 数值绘制：`LCD_Fill` 只清数值行 16px 高带再画，不重绘标题/提示（减闪烁）
- **渲染 mode 一律用 0**（见 Common Mistakes）
- 汉字+数字混排：`LCD_ShowChinese` 画中文（16px/字），`LCD_ShowString` 在其后画 ASCII 数字

### 6. 挂菜单（`code/src/menu.c`，已 GB2312）
- 加 `#include "xxx_view.h"`
- `menu_items[]` 加一行：`{ "模块名", xxx_view_enter, xxx_view_tick, xxx_view_on_event },`
- **导航逻辑零改动**

### 7. `main.c` 初始化（已 GB2312）
- 加 `#include "xxx.h"`
- 在 `joystick_init()` 后调 `xxx_init()`（确保 ADC1 已初始化）

### 8. GB2312 转码（见下文工作流，必做）

### 9. 验证
编译（新 `.c` 手动加 Keil 工程）→ 菜单显示模块名 → SW 进入视图 → 数据刷新 → DOWN 返回

## GB2312 编码工作流（核心，最易出错）

项目源文件**全部 GB2312**（包括 io.c/io.h），无例外。**禁用 sed/awk 改 GB2312 文件**（会损坏多字节字符）。

**新建文件**——Write 写 UTF-8，再 Python 转 GB2312：
```bash
python3 -c "
for p in ['code/inc/xxx.h','code/src/xxx.c','code/inc/xxx_view.h','code/src/xxx_view.c']:
    t=open(p,'rb').read().decode('utf-8')
    try: open(p,'wb').write(t.encode('gb2312'))
    except UnicodeEncodeError: open(p,'wb').write(t.encode('gbk'))
    print(p,'-> OK')
"
```

**改已有 GB2312 文件**——Python 字节级替换，**禁止手写 GB2312 字节**（易错且静默失败）：

**第一步：从目标文件提取实际字节**（绝不全凭记忆手写）：
```bash
python3 -c "
raw=open('code/src/menu.c','rb').read()
# print repr of the line you want to match, then copy-paste into the replace script
for i,l in enumerate(raw.split(b'\n')):
    if b'ledstrip' in l.lower():
        print(f'line {i+1}: {repr(l)}')
"
```

**第二步：用提取的精确字节做替换**（中文用 `'中文'.encode('gb2312')` 生成，或从 repr 直接复制）：
```bash
python3 -c "
raw=open('code/src/menu.c','rb').read()
raw=raw.replace(b'    { \"\xb5\xc6\xb4\xf8\xc4\xa3\xbf\xe9\", ... },\n',   # <-- 从 repr 复制
                b'    { \"\xb5\xc6\xb4\xf8\xc4\xa3\xbf\xe9\", ... },\n'
                b'    { \"\xb7\xe4\xc3\xf9\xc6\xf7\xc4\xa3\xbf\xe9\", ... },\n')
open('code/src/menu.c','wb').write(raw)
"
```

**第三步：替换后必须验证**（确认改动确实落地，避免静默失败）：
```bash
python3 -c "
raw=open('code/src/menu.c','rb').read()
for i,l in enumerate(raw.split(b'\n')):
    if b'beep' in l.lower():
        print(f'line {i+1}: {repr(l)}')
"
```
> 如果验证输出为空，说明 `replace()` 的搜索串没匹配上——回到第一步检查字节。
> bytes 字面量含双引号时，**外层用单引号** `b'#include "x.h"'`，否则闭引号被吞。
> 正则改整行（含行尾中文注释）：`re.compile(b'    xxx_init\\(\\);[^\\n]*')`。
> `$TMPDIR` 未设置 → 全程 Python 内存方式，不用 iconv 临时文件。
> 注意行尾：文件可能是 `\r\n` 或 `\n`，替换时用精确字节匹配。
> `→`(U+2192) 等符号 gb2312 codec 支持（符号区 A1AA）。

**改完读回验证**：
```bash
python3 -c "print(open('code/src/menu.c','rb').read().decode('gb2312'))"
```
（终端按 UTF-8 显示 GB2312 会乱码，属正常；关键是 round-trip 无 UnicodeDecodeError）

## Common Mistakes（踩过的坑）

| 坑 | 现象 | 修复 |
|----|------|------|
| `char line[N]` 未初始化 | `menu_show_line` 读栈垃圾当 GB2312，渲染出多余汉字（如多出"模块"） | sprintf 前 `memset(line,0,sizeof(line))` |
| LCD 渲染 mode=1（叠加逐像素） | 字符错位/残留/重复 | 视图与菜单渲染**一律 mode=0**（非叠加批量写） |
| sed 改 GB2312 文件 | 文件损坏、编译乱码 | Python 字节级替换 |
| 没问硬件就写驱动 | 引脚/信号类型猜错返工 | 先 AskUserQuestion 确认 |
| 没查字库就承诺文案 | 显示缺字/空白方块 | 先 Python 提取 tfont16 清单 |
| DO/触发信号电平反 | 无触发也报警/触发不响应 | 电平做成可配置宏，烧录后验证方向 |
| 视图 tick 阻塞主循环 | DHT 类读取 ~20ms 卡顿 | 节流到 ~1s/次（可接受） |
| MQ-2 类预热 | 上电 1-5 分钟 AO 偏高/DO 误报 | 属正常，等预热，非 bug |

## 项目约定速查

- **编码**：源文件全部 GB2312（含 io.c/io.h），无例外；新代码用中文注释
- **命名**：函数 `snake_case` + 模块前缀（`GPIO_init`/`joystick_event`/`fan_view_enter`）；宏 `UPPER_CASE`（`LED_PIN`/`JOY_MID`）
- **头文件保护**：`#ifndef __MODULE_H`
- **位置**：`.c` 放 `code/src/`，`.h` 放 `code/inc/`；头文件 include `"stm32f10x.h"`
- **无构建系统**：新 `.c` 需手动加 Keil 工程
- ISR 共享变量用 `volatile`；文件内部函数用 `static`
- 硬件宏集中定义（`io.h` 或模块顶部），引脚变动只改宏
