#ifndef __LIGHT_H
#define __LIGHT_H
#include "stm32f10x.h"

/* 光敏模块驱动：AO 模拟亮度(PC3, ADC1_IN13) + DO 数字阈值(PB13)
 * ADC1 由 joystick_init() 初始化，本模块读取通道 13，不重复初始化 ADC1。
 */

/* 初始化引脚：PC3=AIN，PB13=IPU；ADC1 依赖 joystick_init 已就绪 */
void light_init(void);

/* 读 AO 亮度：ADC 原始值 0-4095（4 次平均） */
u16  light_read_ao(void);

/* 读 DO 阈值状态：1=触发（暗），0=正常（亮）；电平逻辑由内部宏消化 */
u8   light_read_alarm(void);

#endif /* __LIGHT_H */
