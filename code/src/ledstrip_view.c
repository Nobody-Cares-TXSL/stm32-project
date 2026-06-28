#include "ledstrip_view.h"
#include "alarm.h"
#include "ledstrip.h"
#include "lcd.h"
#include "joystick.h"
#include <stdio.h>

/* ====== layout (128x128, 16px/row) ====== */
#define LS_Y_TITLE   0
#define LS_Y_S1     28
#define LS_Y_S2     56
#define LS_Y_HINT1  84
#define LS_Y_HINT2 100
#define LS_Y_HINT3 110

static u8 ls_sel = LEDSTRIP_1;

/* mode name + LCD color for text */
static const u8 *mode_name(u8 mode)
{
    switch (mode) {
        case 1: return (u8 *)"YEL";
        case 2: return (u8 *)"ORG";
        case 3: return (u8 *)"RED";
        default: return (u8 *)"OFF";
    }
}

static u16 mode_color(u8 mode)
{
    switch (mode) {
        case 1: return YELLOW;    /* 0xFFE0 */
        case 2: return RED;       /* approximate */
        case 3: return RED;       /* 0xF800 */
        default: return WHITE;
    }
}

/* draw one strip row: "灯带N: X/3 MODE" */
static void ls_draw_strip(u8 strip, u16 y)
{
    char buf[4];
    u8 mode = ledstrip_get_mode(strip);
    u16 fc = (strip == ls_sel) ? BLUE : WHITE;

    LCD_Fill(0, y, 128, y + 16, BLACK);
    LCD_ShowChinese(0, y, (u8 *)"\xB5\xC6\xB4\xF8", fc, BLACK, 16, 0);
    LCD_ShowString(32, y, (u8 *)(strip == LEDSTRIP_1 ? "1:" : "2:"), fc, BLACK, 16, 0);
    sprintf(buf, "%d", mode);
    LCD_ShowString(48, y, (u8 *)buf, fc, BLACK, 16, 0);
    LCD_ShowString(56, y, (u8 *)"/3", WHITE, BLACK, 16, 0);
    LCD_ShowString(80, y, mode_name(mode), mode_color(mode), BLACK, 16, 0);
}

void ledstrip_view_enter(void)
{
    LCD_Fill(0, 0, 128, 128, BLACK);
    LCD_ShowChinese(0, LS_Y_TITLE, (u8 *)"\xB5\xC6\xB4\xF8\xC4\xA3\xBF\xE9", WHITE, BLACK, 16, 0);
    ls_draw_strip(LEDSTRIP_1, LS_Y_S1);
    ls_draw_strip(LEDSTRIP_2, LS_Y_S2);
    LCD_ShowString(0, LS_Y_HINT1, (u8 *)"L/R:MODE", WHITE, BLACK, 16, 0);
    LCD_ShowString(0, LS_Y_HINT2, (u8 *)"SW:SEL", WHITE, BLACK, 16, 0);
    LCD_ShowString(0, LS_Y_HINT3, (u8 *)"DN:BACK", WHITE, BLACK, 16, 0);
    alarm_manual_override(ALARM_MANUAL_LEDSTRIP);
}

void ledstrip_view_tick(void)
{
    /* ledstrip_tick 由 alarm 引擎统一驱动（任何视图态都闪） */
}

u8 ledstrip_view_on_event(u8 ev, u8 sw)
{
    u8 mode;

    if (ev == JOY_DOWN) { alarm_manual_release(ALARM_MANUAL_LEDSTRIP); return 1; }

    /* SW: switch selected strip */
    if (sw) {
        ls_sel = (ls_sel == LEDSTRIP_1) ? LEDSTRIP_2 : LEDSTRIP_1;
        ls_draw_strip(LEDSTRIP_1, LS_Y_S1);
        ls_draw_strip(LEDSTRIP_2, LS_Y_S2);
        return 0;
    }

    mode = ledstrip_get_mode(ls_sel);

    if (ev == JOY_RIGHT) {
        if (mode < LS_MODE_MAX) {
            ledstrip_set_mode(ls_sel, mode + 1);
            ls_draw_strip(ls_sel, (ls_sel == LEDSTRIP_1) ? LS_Y_S1 : LS_Y_S2);
        }
        return 0;
    }

    if (ev == JOY_LEFT) {
        if (mode > 0) {
            ledstrip_set_mode(ls_sel, mode - 1);
            ls_draw_strip(ls_sel, (ls_sel == LEDSTRIP_1) ? LS_Y_S1 : LS_Y_S2);
        }
        return 0;
    }

    return 0;
}
