#include "stm32f10x.h"
#include "stdio.h"		// printf
#include "string.h"		// strcmp, memset
#include "io.h"
#include "delay.h"
#include "lcd_ui.h"
#include "lcd.h"

int main() {
	
    // 初始化 SysTick 为 HCLK/8
    SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK_Div8);

    // 初始化 屏幕UI
    ui_init();
    delay_ms(100);
    // 登录提示
    ui_login("欢迎进入消防预测系统");
    delay_ms(250);

    //	清空屏幕		屏幕大小128*128	颜色填充
	LCD_Fill(0, 0, 128, 128, GRED);

	while (1) {
		
	}
}
