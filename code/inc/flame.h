#ifndef __FLAME_H
#define __FLAME_H
#include "stm32f10x.h"

/* 火焰传感器模块驱动：两个 AO+DO 传感器，共享 ADC1
 *
 * 传感器 1: PC1(AO, ADC1_IN11) + PB11(DO)
 * 传感器 2: PC2(AO, ADC1_IN12) + PB12(DO)
 *
 * ADC1 由 joystick_init() 初始化，本模块复用读取。
 */

#define FLAME_SENSOR_1  0
#define FLAME_SENSOR_2  1

void flame_init(void);                    /* 初始化两个传感器的 AO/DO 引脚 */
u16  flame_read_ao(u8 sensor);            /* 读指定传感器 AO（0-4095） */
u8   flame_read_alarm(u8 sensor);         /* 读指定传感器 DO（1=火警） */

#endif /* __FLAME_H */
