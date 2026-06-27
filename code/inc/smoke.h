#ifndef __SMOKE_H
#define __SMOKE_H
#include "stm32f10x.h"

/* 烟雾报警器模块驱动（AO 模拟浓度 + DO 数字报警）
 * AO 接 PC0（ADC1_IN10），DO 接 PB10（GPIO 输入）。
 * ADC1 由 joystick_init() 初始化并校准，本模块仅读取通道，不重复初始化 ADC1。
 */

/* 初始化烟雾引脚：PC0=AIN、PB10=IPU（ADC1 已由 joystick_init 就绪） */
void smoke_init(void);

/* 读 AO 浓度：返回 ADC 原始值 0-4095（4 次平均，抗噪） */
u16  smoke_read_ao(void);

/* 读 DO 报警状态：1=报警，0=安全（电平极性内部消化，调用方无需关心） */
u8   smoke_read_alarm(void);

#endif /* __SMOKE_H */
