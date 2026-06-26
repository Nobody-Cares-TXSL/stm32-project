#include "lcd_ui.h"
#include "lcd.h"			/* LCD_Fill / LCD_ShowString + 颜色宏 */
#include "lcd_init.h"		/* LCD_Init */

/* LCD 初始化 + 黑底清屏 */
void ui_init(void)
{
	LCD_Init();
	LCD_Fill(0, 0, 128, 128, BLACK);
}

/* 登录界面：标题 + 一行提示 msg */
void ui_login(const char *msg)
{
	LCD_Fill(0, 0, 128, 128, BLACK);
	LCD_ShowChinese(0,0,(u8 *)msg,RED,DARKBLUE,32,0);
	LCD_ShowChinese(32,48,(u8 *)"消防",RED,DARKBLUE,32,0);
	LCD_ShowChinese(0,84,(u8 *)"预测系统",RED,DARKBLUE,32,0);
}


