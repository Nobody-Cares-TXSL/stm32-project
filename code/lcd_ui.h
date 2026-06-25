#ifndef __LCD_UI_H
#define __LCD_UI_H

#include "stm32f10x.h"

/* LCD 界面绘制层：集中所有屏幕显示，main/menu 只调这些 */
void ui_init(void);					/* LCD_Init + 黑底清屏 */
void ui_login(const char *msg);		/* 登录界面：标题 + msg */

#endif /* __LCD_UI_H */
