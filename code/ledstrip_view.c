#include "ledstrip_view.h"
#include "ledstrip.h"  /* ledstrip_set_count / ledstrip_get_count / LEDSTRIP_1/2 / LEDSTRIP_MAX */
#include "lcd.h"       /* LCD_Fill / LCD_ShowChinese / LCD_ShowString + BLACK/WHITE/BLUE */
#include "joystick.h"  /* JOY_DOWN / JOY_LEFT / JOY_RIGHT */
#include <stdio.h>     /* sprintf */

/* ====== 布局（128x128，16px/行） ====== */
#define LS_Y_TITLE   0      /* 标题 */
#define LS_Y_S1     28      /* 灯带 1 */
#define LS_Y_S2     56      /* 灯带 2 */
#define LS_Y_HINT1  84      /* 操作提示 */
#define LS_Y_HINT2 100      /* 操作提示 */
#define LS_Y_HINT3 110      /* 返回提示 */

static u8 ls_sel = LEDSTRIP_1;    /* 当前选中的灯带 */

/* 画一条灯带行：清行 -> "灯带N:" -> "XX" -> "/30" */
static void ls_draw_strip(u8 strip, u16 y)
{
    char buf[8];
    u8 count = ledstrip_get_count(strip);
    u16 fc = (strip == ls_sel) ? BLUE : WHITE;   /* 选中行蓝色 */

    LCD_Fill(0, y, 128, y + 16, BLACK);
    LCD_ShowChinese(0, y, (u8 *)"灯带", fc, BLACK, 16, 0);
    LCD_ShowString(32, y, (u8 *)(strip == LEDSTRIP_1 ? "1:" : "2:"), fc, BLACK, 16, 0);
    sprintf(buf, "%d", count);
    LCD_ShowString(48, y, (u8 *)buf, fc, BLACK, 16, 0);
    LCD_ShowString(56, y, (u8 *)"/", WHITE, BLACK, 16, 0);
    sprintf(buf, "%d", (strip == LEDSTRIP_1) ? LS1_MAX : LS2_MAX);
    LCD_ShowString(64, y, (u8 *)buf, WHITE, BLACK, 16, 0);
}

void ledstrip_view_enter(void)
{
    LCD_Fill(0, 0, 128, 128, BLACK);
    LCD_ShowChinese(0, LS_Y_TITLE, (u8 *)"灯带模块", WHITE, BLACK, 16, 0);
    ls_draw_strip(LEDSTRIP_1, LS_Y_S1);
    ls_draw_strip(LEDSTRIP_2, LS_Y_S2);
    LCD_ShowString(0, LS_Y_HINT1, (u8 *)"L/R:COUNT", WHITE, BLACK, 16, 0);
    LCD_ShowString(0, LS_Y_HINT2, (u8 *)"SW:SEL", WHITE, BLACK, 16, 0);
    LCD_ShowString(0, LS_Y_HINT3, (u8 *)"DN:BACK", WHITE, BLACK, 16, 0);
}

void ledstrip_view_tick(void)
{
    /* 状态仅在用户操作时变化，无需周期刷新 */
}

u8 ledstrip_view_on_event(u8 ev, u8 sw)
{
    u8 count;

    if (ev == JOY_DOWN) return 1;

    /* SW：切换选中灯带 */
    if (sw) {
        ls_sel = (ls_sel == LEDSTRIP_1) ? LEDSTRIP_2 : LEDSTRIP_1;
        ls_draw_strip(LEDSTRIP_1, LS_Y_S1);
        ls_draw_strip(LEDSTRIP_2, LS_Y_S2);
        return 0;
    }

    count = ledstrip_get_count(ls_sel);

    if (ev == JOY_RIGHT) {
        if (count < ((ls_sel == LEDSTRIP_1) ? LS1_MAX : LS2_MAX)) {
            ledstrip_set_count(ls_sel, count + 1);
            ls_draw_strip(ls_sel, (ls_sel == LEDSTRIP_1) ? LS_Y_S1 : LS_Y_S2);
        }
        return 0;
    }

    if (ev == JOY_LEFT) {
        if (count > 0) {
            ledstrip_set_count(ls_sel, count - 1);
            ls_draw_strip(ls_sel, (ls_sel == LEDSTRIP_1) ? LS_Y_S1 : LS_Y_S2);
        }
        return 0;
    }

    return 0;
}
