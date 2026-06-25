#ifndef __DELAY_H
#define __DELAY_H

#include "stm32f10x.h"

void delay_us(u32 nus);
void delay_ms(u32 ms);
void delay_ms_chk(u32 ms);	/* 可被中断打断的延时：每 1ms 检查 state_dirty */

#endif /* __DELAY_H */
