#include "flame_view.h"
#include "flame.h"
#include "alarm.h"
#include "lcd.h"       /* LCD_Fill / LCD_ShowChinese / LCD_ShowString + BLACK/WHITE/RED/GREEN */
#include "joystick.h"  /* JOY_DOWN */
#include <stdio.h>     /* sprintf */

/* ====== 刷新节流（0.5s） ====== */
#define FLAME_REFRESH_TICKS  50

/* ====== 布局（128x128，16px/行，双传感器组合） ====== */
#define FM_Y_TITLE   0      /* 标题 */
#define FM_Y_AO1    20      /* S1 AO */
#define FM_Y_DO1    36      /* S1 DO */
#define FM_Y_AO2    56      /* S2 AO */
#define FM_Y_DO2    72      /* S2 DO */
#define FM_Y_HINT  110      /* 返回提示 */

/* SW alarm threshold: ADC below this = fire (0=HW DO only); larger=more sensitive */
#define FLAME_SW_THRESHOLD  3000

static u16 fm_ao[2]    = {0, 0};
static u8  fm_alarm[2] = {0, 0};
static u16 fm_tick     = 0;

/* 画单行 AO：清行 -> "火焰N:" -> "XX%" */
static void fm_draw_ao(u8 sensor, u16 y)
{
    char buf[8];
    u16 pct = fm_ao[sensor];
    LCD_Fill(0, y, 128, y + 16, BLACK);
    LCD_ShowChinese(0, y, (u8 *)"火焰", WHITE, BLACK, 16, 0);  /* 32px */
    LCD_ShowString(32, y, (u8 *)(sensor ? "2:" : "1:"), WHITE, BLACK, 16, 0);  /* 16px */
    sprintf(buf, "%d", pct);
    LCD_ShowString(48, y, (u8 *)buf, WHITE, BLACK, 16, 0);
}

/* 画单行 DO：清行 -> "DO1/2:" -> "报警"/"SAFE" */
static void fm_draw_do(u8 sensor, u16 y)
{
    LCD_Fill(0, y, 128, y + 16, BLACK);
    LCD_ShowString(0, y, (u8 *)(sensor ? "DO2:" : "DO1:"), WHITE, BLACK, 16, 0);
    if (fm_ao[sensor] < FLAME_SW_THRESHOLD)
        LCD_ShowChinese(32, y, (u8 *)"报警", RED, BLACK, 16, 0);
    else
        LCD_ShowString(32, y, (u8 *)"SAFE", GREEN, BLACK, 16, 0);
}

void flame_view_enter(void)
{
    fm_tick = FLAME_REFRESH_TICKS;   /* 强制首次 tick 立即读取 */

    LCD_Fill(0, 0, 128, 128, BLACK);
    LCD_ShowChinese(0, FM_Y_TITLE, (u8 *)"火焰传感器", WHITE, BLACK, 16, 0);
    LCD_ShowString(0, FM_Y_HINT, (u8 *)"DOWN:BACK", WHITE, BLACK, 16, 0);

    /* 占位 */
    LCD_ShowChinese(0, FM_Y_AO1, (u8 *)"火焰", WHITE, BLACK, 16, 0);
    LCD_ShowString(32, FM_Y_AO1, (u8 *)"1:--", WHITE, BLACK, 16, 0);
    LCD_ShowString(0, FM_Y_DO1, (u8 *)"DO1:--", WHITE, BLACK, 16, 0);
    LCD_ShowChinese(0, FM_Y_AO2, (u8 *)"火焰", WHITE, BLACK, 16, 0);
    LCD_ShowString(32, FM_Y_AO2, (u8 *)"2:--", WHITE, BLACK, 16, 0);
    LCD_ShowString(0, FM_Y_DO2, (u8 *)"DO2:--", WHITE, BLACK, 16, 0);
}

void flame_view_tick(void)
{
    if (++fm_tick < FLAME_REFRESH_TICKS) return;
    fm_tick = 0;

    fm_ao[0] = alarm_get_flame_n(1);
    fm_ao[1] = alarm_get_flame_n(2);
    fm_draw_ao(FLAME_SENSOR_1, FM_Y_AO1);
    fm_draw_do(FLAME_SENSOR_1, FM_Y_DO1);
    fm_draw_ao(FLAME_SENSOR_2, FM_Y_AO2);
    fm_draw_do(FLAME_SENSOR_2, FM_Y_DO2);
}

u8 flame_view_on_event(u8 ev, u8 sw)
{
    if (ev == JOY_DOWN) return 1;
    return 0;
}
