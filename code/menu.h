#ifndef __MENU_H
#define __MENU_H

#include "stm32f10x.h"

/* 单个菜单项（加功能时在此补回调字段，如 void (*on_enter)(void)） */
typedef struct {
    const char *text;
} menu_item_t;

/* 初始化菜单并绘制初始画面（光标在第 0 行，无确认项） */
void menu_init(void);

/* 喂入一轮轮询事件：ev=joystick_event() 返回值, sw=joystick_pressed()
   上下移动光标(白色 "> " 标记), SW 确认当前行(文字变蓝) */
void menu_handle(u8 ev, u8 sw);

#endif /* __MENU_H */
