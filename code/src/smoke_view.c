#include "smoke_view.h"
#include "alarm.h"
#include "lcd.h"       /* LCD_Fill / LCD_ShowChinese / LCD_ShowString + BLACK/WHITE/RED/GREEN */
#include "joystick.h"  /* JOY_DOWN */
#include <stdio.h>     /* sprintf */

/* ====== 刷新节流（主循环约 10ms，50 ≈ 0.5s；ADC 快，故比温湿度的 1s 更频繁） ====== */
#define SMOKE_REFRESH_TICKS  50

/* ====== 布局（128×128，16px/行） ====== */
#define SM_Y_TITLE   0      /* 标题：烟雾报警器 */
#define SM_Y_AO     28      /* AO 浓度数值 */
#define SM_Y_DO     56      /* DO 报警状态 */
#define SM_Y_HINT  110      /* 返回提示（ASCII，字库无"返回"二字） */

/* ====== 数值缓存 ====== */
static u16 sm_ao    = 0;    /* AO 原始值 0-4095 */
static u8  sm_alarm = 0;    /* DO 报警：1/0 */
static u16 sm_tick  = 0;    /* 刷新计数器 */

/* AO 行：清行后画"烟雾:百分比%"（百分比 = ADC 满量程比例，非标定 ppm） */
static void sm_draw_ao(u16 adc)
{
    char buf[8];
    u8 pct = (u8)((u32)adc * 100 / 4096);
    LCD_Fill(0, SM_Y_AO, 128, SM_Y_AO + 16, BLACK);
    LCD_ShowChinese(0, SM_Y_AO, (u8 *)"烟雾", WHITE, BLACK, 16, 0);   /* 2 字 = 32px */
    sprintf(buf, ":%d%%", pct);
    LCD_ShowString(32, SM_Y_AO, (u8 *)buf, WHITE, BLACK, 16, 0);
}

/* DO 行：清行后画"DO:"+状态（报警红 / 安全绿） */
static void sm_draw_do(u8 alarm)
{
    LCD_Fill(0, SM_Y_DO, 128, SM_Y_DO + 16, BLACK);
    LCD_ShowString(0, SM_Y_DO, (u8 *)"DO:", WHITE, BLACK, 16, 0);     /* 3 字符 = 24px */
    if (alarm) LCD_ShowChinese(24, SM_Y_DO, (u8 *)"报警", RED, BLACK, 16, 0);
    else       LCD_ShowString(24, SM_Y_DO, (u8 *)"SAFE", GREEN, BLACK, 16, 0);
}

void smoke_view_enter(void)
{
    sm_tick = SMOKE_REFRESH_TICKS;   /* 强制首次 tick 立即读一次 */

    LCD_Fill(0, 0, 128, 128, BLACK);
    LCD_ShowChinese(0, SM_Y_TITLE, (u8 *)"烟雾报警器", WHITE, BLACK, 16, 0);
    LCD_ShowString(0, SM_Y_HINT, (u8 *)"DOWN:BACK", WHITE, BLACK, 16, 0);

    /* 占位：未读到前显示 --，不预显 SAFE/报警 */
    LCD_ShowChinese(0, SM_Y_AO, (u8 *)"烟雾", WHITE, BLACK, 16, 0);
    LCD_ShowString(32, SM_Y_AO, (u8 *)":--", WHITE, BLACK, 16, 0);
    LCD_ShowString(0, SM_Y_DO, (u8 *)"DO:--", WHITE, BLACK, 16, 0);
}

void smoke_view_tick(void)
{
    if (++sm_tick < SMOKE_REFRESH_TICKS) return;   /* 约 0.5s 刷新一次 */
    sm_tick = 0;

    sm_ao    = alarm_get_smoke();
    sm_alarm = (alarm_get_smoke_level() >= 1) ? 1 : 0;
    sm_draw_ao(sm_ao);
    sm_draw_do(sm_alarm);
}

u8 smoke_view_on_event(u8 ev, u8 sw)
{
    if (ev == JOY_DOWN) return 1;     /* 下拨 → 退回主菜单 */
    return 0;
}
