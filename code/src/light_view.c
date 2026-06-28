#include "light_view.h"
#include "alarm.h"
#include "lcd.h"       /* LCD_Fill / LCD_ShowChinese / LCD_ShowString + BLACK/WHITE/RED/GREEN */
#include "joystick.h"  /* JOY_DOWN */
#include <stdio.h>     /* sprintf */

/* ====== 刷新节流（主循环约 10ms，50 次 = 0.5s；ADC 快，可较频繁） ====== */
#define LIGHT_REFRESH_TICKS  50

/* ====== 布局（128x128，16px/行） ====== */
#define LT_Y_TITLE   0      /* 标题 */
#define LT_Y_AO     28      /* AO 亮度值 */
#define LT_Y_DO     56      /* DO 阈值状态 */
#define LT_Y_HINT  110      /* 返回提示（用 ASCII 规避"返回"缺字） */

/* ====== 数值缓存 ====== */
static u16 lt_ao    = 0;    /* AO 原始值 0-4095 */
static u8  lt_alarm = 0;    /* DO 触发：1/0 */
static u16 lt_tick  = 0;    /* 刷新计数器 */

/* AO 行：清行 -> "光敏" -> ":XX%" */
static void lt_draw_ao(u16 adc)
{
    char buf[8];
    u8 pct = (u8)((u32)(4096 - adc) * 100 / 4096);
    LCD_Fill(0, LT_Y_AO, 128, LT_Y_AO + 16, BLACK);
    LCD_ShowChinese(0, LT_Y_AO, (u8 *)"光敏", WHITE, BLACK, 16, 0);   /* 2 字 = 32px */
    sprintf(buf, ":%d%%", pct);
    LCD_ShowString(32, LT_Y_AO, (u8 *)buf, WHITE, BLACK, 16, 0);
}

/* DO 行：清行 -> "DO:" -> "报警"(红) 或 "SAFE"(绿) */
static void lt_draw_do(u8 alarm)
{
    LCD_Fill(0, LT_Y_DO, 128, LT_Y_DO + 16, BLACK);
    LCD_ShowString(0, LT_Y_DO, (u8 *)"DO:", WHITE, BLACK, 16, 0);     /* 3 字符 = 24px */
    if (alarm) LCD_ShowChinese(24, LT_Y_DO, (u8 *)"报警", RED, BLACK, 16, 0);
    else       LCD_ShowString(24, LT_Y_DO, (u8 *)"SAFE", GREEN, BLACK, 16, 0);
}

void light_view_enter(void)
{
    lt_tick = LIGHT_REFRESH_TICKS;   /* 强制首次 tick 立即读取 */

    LCD_Fill(0, 0, 128, 128, BLACK);
    LCD_ShowChinese(0, LT_Y_TITLE, (u8 *)"光敏传感器", WHITE, BLACK, 16, 0);
    LCD_ShowString(0, LT_Y_HINT, (u8 *)"DOWN:BACK", WHITE, BLACK, 16, 0);

    /* 占位：数据未读到前显示 -- */
    LCD_ShowChinese(0, LT_Y_AO, (u8 *)"光敏", WHITE, BLACK, 16, 0);
    LCD_ShowString(32, LT_Y_AO, (u8 *)":--", WHITE, BLACK, 16, 0);
    LCD_ShowString(0, LT_Y_DO, (u8 *)"DO:--", WHITE, BLACK, 16, 0);
}

void light_view_tick(void)
{
    if (++lt_tick < LIGHT_REFRESH_TICKS) return;   /* 约 0.5s 刷新一次 */
    lt_tick = 0;

    lt_ao    = alarm_get_light();
    lt_alarm = (alarm_get_light_level() >= 1) ? 1 : 0;
    lt_draw_ao(lt_ao);
    lt_draw_do(lt_alarm);
}

u8 light_view_on_event(u8 ev, u8 sw)
{
    if (ev == JOY_DOWN) return 1;     /* 下拨 -> 返回主菜单 */
    return 0;
}
