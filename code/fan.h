#ifndef __FAN_H
#define __FAN_H
#include "stm32f10x.h"

/* 风扇模块驱动：PB0 (TIM3_CH3) PWM 调速控制
 * TIM3 由本模块独立初始化，不依赖 ADC1/摇杆
 */

void fan_init(void);          /* 初始化 TIM3 CH3 PWM 输出 */
void fan_set_speed(u8 pct);   /* 设置占空比 0-100 (0=停止, 100=全速) */
u8   fan_get_speed(void);     /* 获取当前占空比 */

#endif /* __FAN_H */
