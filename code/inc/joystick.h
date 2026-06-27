#ifndef __JOYSTICK_H
#define __JOYSTICK_H

#include "stm32f10x.h"

/* ====== 摇杆方向码 ====== */
#define JOY_MID    0
#define JOY_UP     1
#define JOY_DOWN   2
#define JOY_LEFT   3
#define JOY_RIGHT  4

/* 初始化 ADC(X=PB1, Y=PC4) 和 SW 引脚(PC5)，轮询前调用一次 */
void joystick_init(void);

/* 摇杆方向边沿事件：仅从中点推出时返回方向码，否则返回 JOY_MID */
u8 joystick_event(void);

/* 摇杆按下(SW)边沿事件：按下时返回 1，否则 0（非阻塞消抖） */
u8 joystick_pressed(void);

/* DEBUG: return raw ADC values for diagnosis */
void joystick_debug_adc(u16 *x, u16 *y);

#endif /* __JOYSTICK_H */
