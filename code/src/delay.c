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

/**
 * @brief  秒级延时函数
 * @param  sec 需要延时的秒数
 * @retval 无
 */
void delay_s(u32 sec)
{
    while (sec--) {
        delay_ms(1000); // 循环调用1000次1ms延时
    }
}


