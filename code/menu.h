#ifndef __MENU_H
#define __MENU_H

#include "stm32f10x.h"

/* 菜单项：text=显示文字；enter/tick/on_event 为可选的模块视图回调（全 NULL=占位项） */
typedef struct {
    const char *text;                    /* 显示文字（ASCII 或 GB2312 汉字） */
    void (*enter)(void);                 /* 进入模块视图时调用（首次绘制） */
    void (*tick)(void);                  /* 主循环周期调用（刷新数据） */
    u8   (*on_event)(u8 ev, u8 sw);      /* 视图内输入处理；返回1=退出回主菜单 */
} menu_item_t;

/* 初始化菜单并绘制初始画面（光标在第 0 行，主菜单态） */
void menu_init(void);

/* 喂入一轮轮询事件：ev=joystick_event() 返回值, sw=joystick_pressed()
   主菜单态：上下移动光标，SW 进入模块视图（占位项则确认变蓝）
   模块视图态：周期 tick 刷新，on_event 返回1 则退回主菜单 */
void menu_handle(u8 ev, u8 sw);

#endif /* __MENU_H */
