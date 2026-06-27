#ifndef __LEDSTRIP_H
#define __LEDSTRIP_H
#include "stm32f10x.h"

/* 灯带模块驱动：WS2812B 幻彩灯带，DIN 引脚软件位脉冲协议
 * 灯带 1: PC12(DIN)，灯带 2: PC11(DIN)
 * 最大灯珠数可配，默认 30
 */

#define LEDSTRIP_1      0
#define LEDSTRIP_2      1
#define LS1_MAX  7               /* strip 1 max */
#define LS2_MAX  9               /* strip 2 max */

void ledstrip_init(void);                    /* 初始化两个 DIN 引脚 */
void ledstrip_set_count(u8 strip, u8 count); /* 点亮前 count 个灯珠（白），其余灭 */
u8   ledstrip_get_count(u8 strip);           /* 查询当前点亮数 */

#endif /* __LEDSTRIP_H */
