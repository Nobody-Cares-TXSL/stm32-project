#include "menu.h"
#include "joystick.h"   /* JOY_UP / JOY_DOWN */
#include "lcd.h"        /* LCD_Fill / LCD_ShowChar / LCD_ShowChinese + BLACK/WHITE/BLUE */
#include "th_view.h"    /* 温湿度模块视图回调 */
#include "smoke_view.h"   /* 烟雾模块视图回调 */
#include "fan_view.h"   /* fan module */
#include <string.h>     /* memset */
#include <stdio.h>      /* sprintf */

/* ====== 菜单项表：增删菜单、挂模块只动这里 ====== */
static const menu_item_t menu_items[] = {
    { "温湿度模块", th_view_enter, th_view_tick, th_view_on_event },  /* 点击进入数据视图 */
    { "烟雾报警器", smoke_view_enter, smoke_view_tick, smoke_view_on_event },   /* 烟雾模块 */
    { "风扇模块", fan_view_enter, fan_view_tick, fan_view_on_event },   /* fan module */
    { "iop",        NULL, NULL, NULL },
};
#define MENU_COUNT (sizeof(menu_items) / sizeof(menu_items[0]))

/* ====== 布局参数 ====== */
#define MENU_ROW_H  16    /* 每行像素高度（16 字号） */
#define MENU_X      0     /* 左侧 x 坐标 */
#define MENU_PREFIX "> "  /* 光标前缀 */

/* ====== 菜单状态 ====== */
static u8 menu_cursor    = 0;            /* 当前光标行 */
static u8 menu_confirmed = MENU_COUNT;   /* 已确认行；MENU_COUNT 表示无确认 */
static const menu_item_t *active_view = NULL;   /* 活动视图；NULL=主菜单态 */

/* 画一行文字：自动区分 ASCII(半角8px) 与 GB2312 汉字(全角16px)；mode=1 叠加不填背景 */
static void menu_show_line(u16 x, u16 y, const u8 *s, u16 fc, u16 bc)
{
    while (*s) {
        if (*s < 0x80) {                        /* ASCII 单字节 */
            LCD_ShowChar(x, y, *s, fc, bc, 16, 0);
            x += 8;
            s += 1;
        } else {                                /* GB2312 双字节汉字 */
            u8 ch[3] = { s[0], s[1], 0 };
            LCD_ShowChinese(x, y, ch, fc, bc, 16, 0);
            x += 16;
            s += 2;
        }
    }
}

/* 重绘整屏主菜单：黑底、白字、确认行蓝字、光标行带 "> " */
static void menu_draw(void)
{
    u8 i;
    LCD_Fill(0, 0, 128, 128, BLACK);
    for (i = 0; i < MENU_COUNT; i++) {
        char line[32];
        memset(line, 0, sizeof(line));
        u16 fc = (i == menu_confirmed) ? BLUE : WHITE;
        sprintf(line, "%s%s", (i == menu_cursor) ? MENU_PREFIX : "  ", menu_items[i].text);
        menu_show_line(MENU_X, i * MENU_ROW_H, (const u8 *)line, fc, BLACK);
    }
}

void menu_init(void)
{
    menu_cursor    = 0;
    menu_confirmed = MENU_COUNT;
    active_view    = NULL;
    menu_draw();
}

/**
 * @brief 菜单处理：主菜单导航 / 模块视图分发
 * @param ev 摇杆方向事件（JOY_UP/DOWN/MID 等）
 * @param sw 摇杆按下事件（非0=按下）
 */
void menu_handle(u8 ev, u8 sw)
{
    u8 dirty = 0;

    /* 模块视图态：周期刷新 + 分发输入；on_event 返回1 则退回主菜单 */
    if (active_view != NULL) {
        active_view->tick();
        if (active_view->on_event(ev, sw)) {
            active_view = NULL;
            menu_draw();
        }
        return;
    }

    /* 主菜单态：上下循环移动光标 */
    if (ev == JOY_UP) {
        menu_cursor = (menu_cursor + MENU_COUNT - 1) % MENU_COUNT;
        dirty = 1;
    } else if (ev == JOY_DOWN) {
        menu_cursor = (menu_cursor + 1) % MENU_COUNT;
        dirty = 1;
    }

    /* SW：有视图回调→进入模块；否则确认变蓝（占位项） */
    if (sw) {
        if (menu_items[menu_cursor].enter != NULL) {
            active_view = &menu_items[menu_cursor];
            active_view->enter();   /* enter 自行绘制视图 */
            return;
        }
        menu_confirmed = menu_cursor;
        dirty = 1;
    }

    if (dirty) menu_draw();
}

