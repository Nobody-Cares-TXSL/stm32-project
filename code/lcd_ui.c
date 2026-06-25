#include "lcd_ui.h"
#include "lcd.h"			/* LCD_Fill / LCD_ShowString + 颜色宏 */
#include "lcd_init.h"		/* LCD_Init */

/* 主菜单 4 项文字 */
static const char *menu_items[4] = {"1 LED", "2 Query", "3 Buzzer", "4 Passwd"};
/* 功能模式标题（下标 1~4 对应 effect_index） */
static const char *mode_titles[5] = {"", "LED", "Query", "Buzzer", "Passwd"};

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
}


