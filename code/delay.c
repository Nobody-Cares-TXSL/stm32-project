#include "delay.h"
/* ====== SysTick 时钟延时 ====== */
/* 72MHz / 8 = 9MHz，每计数 1 次 = 1/9 微秒 */
void delay_us(u32 nus)
{
	u32 temp;
	SysTick->LOAD = 9 * nus;
	SysTick->VAL = 0;
	SysTick->CTRL |= 0x01;
	do {
		temp = SysTick->CTRL;
	} while (temp & 0x01 && !(temp & (1 << 16)));
	SysTick->CTRL &= ~0x01;
	SysTick->VAL = 0;
}
void delay_ms(u32 ms)
{
	while (ms--) {
		delay_us(1000);
	}
}
/* 可被中断打断的毫秒延时：每 1ms 检查 state_dirty */
void delay_ms_chk(u32 ms)
{
	while (ms--) {
		delay_us(1000);
	}
}
