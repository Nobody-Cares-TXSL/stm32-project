#ifndef __PUMP_H
#define __PUMP_H
#include "stm32f10x.h"

/* 水泵模块驱动：PA1 GPIO 输出 → 继电器 IN1 → 水泵
 * 继电器模块：IN1=HIGH → 继电器吸合 → 水泵转（默认假设，可改宏）
 */

void pump_init(void);          /* 初始化 PA1 推挽输出，默认关 */
void pump_on(void);            /* 开启水泵 */
void pump_off(void);           /* 关闭水泵 */
u8   pump_is_on(void);         /* 查询状态：1=开，0=关 */

#endif /* __PUMP_H */
