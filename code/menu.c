#include "menu.h"
#include "joystick.h"   /* JOY_UP / JOY_DOWN */
#include "lcd.h"        /* LCD_Fill / LCD_ShowString + BLACK/WHITE/BLUE */
#include <stdio.h>      /* sprintf */

/* ====== 菜单项表：增删菜单只动这里 ====== */
static const menu_item_t menu_items[] = {
    { "qwe" },
    { "asd" },
    { "zxc" },
    { "iop" },
};
#define MENU_COUNT (sizeof(menu_items) / sizeof(menu_items[0]))

/* ====== 布局参数 ====== */
#define MENU_ROW_H  16    /* 每行像素高度（16 字号） */
#define MENU_X      0     /* 左侧 x 坐标 */
#define MENU_PREFIX "> "  /* 光标标记 */

/* ====== 菜单状态 ====== */
static u8 menu_cursor    = 0;            /* 当前光标行 */
static u8 menu_confirmed = MENU_COUNT;   /* 已确认行；MENU_COUNT 表示无确认 */

/* 重绘整屏菜单：黑底、白字、确认行蓝字、光标行前缀 "> " */
static void menu_draw(void)
{
    u8 i;
    LCD_Fill(0, 0, 128, 128, BLACK);
    for (i = 0; i < MENU_COUNT; i++) {
        char line[24];
        u16 fc = (i == menu_confirmed) ? BLUE : WHITE;
        sprintf(line, "%s%s", (i == menu_cursor) ? MENU_PREFIX : "  ", menu_items[i].text);
        LCD_ShowString(MENU_X, i * MENU_ROW_H, (u8 *)line, fc, BLACK, 16, 1);
    }
}

void menu_init(void)
{
    menu_cursor    = 0;
    menu_confirmed = MENU_COUNT;
    menu_draw();
}

/**
 * @brief 菜单处理函数，根据按键事件和开关状态更新菜单光标及确认项
 * 
 * @param ev 按键事件类型（如：JOY_UP向上，JOY_DOWN向下）
 * @param sw 开关状态（非0表示确认选择，0表示无确认操作）
 */
void menu_handle(u8 ev, u8 sw)
{
    // 脏标记，用于指示菜单状态是否发生变化需要重绘，0表示无变化，1表示有变化
    u8 dirty = 0;
    
    // 处理向上按键事件：光标循环上移一位
    if (ev == JOY_UP) {
        // 利用取模运算实现循环移动，加MENU_COUNT防止减1后出现负数
        menu_cursor = (menu_cursor + MENU_COUNT - 1) % MENU_COUNT;
        // 标记状态已改变，需要重绘
        dirty = 1;
    } 
    // 处理向下按键事件：光标循环下移一位
    else if (ev == JOY_DOWN) {
        // 利用取模运算实现循环移动
        menu_cursor = (menu_cursor + 1) % MENU_COUNT;
        // 标记状态已改变，需要重绘
        dirty = 1;
    }
    
    // 处理确认操作：如果开关被触发（非0）
    if (sw) {
        /* 单选：新确认覆盖旧确认 */
        menu_confirmed = menu_cursor;
        // 确认后状态改变，标记需要重绘
        dirty = 1;
    }
    
    // 如果菜单状态有更新（脏标记为1），则调用绘制函数刷新菜单显示
    if (dirty) menu_draw();
}

