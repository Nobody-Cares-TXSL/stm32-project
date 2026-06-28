#include "pump_view.h"
#include "alarm.h"
#include "pump.h"      /* pump_on / pump_off / pump_is_on */
#include "lcd.h"       /* LCD_Fill / LCD_ShowChinese / LCD_ShowString + BLACK/WHITE/RED/GREEN */
#include "joystick.h"  /* JOY_DOWN */

/* ====== 布局（128x128，16px/行） ====== */
#define PU_Y_TITLE   0      /* 标题 */
#define PU_Y_STATE  40      /* PUMP ON/OFF 状态 */
#define PU_Y_HINT1  84      /* 操作提示 */
#define PU_Y_HINT2 110      /* 返回提示 */

/* 状态行绘制 */
static void pu_draw_state(void)
{
    LCD_Fill(0, PU_Y_STATE, 128, PU_Y_STATE + 16, BLACK);
    LCD_ShowString(0, PU_Y_STATE, (u8 *)"PUMP:", WHITE, BLACK, 16, 0);
    if (pump_is_on())
        LCD_ShowString(40, PU_Y_STATE, (u8 *)"ON", GREEN, BLACK, 16, 0);
    else
        LCD_ShowString(40, PU_Y_STATE, (u8 *)"OFF", RED, BLACK, 16, 0);
}

void pump_view_enter(void)
{
    LCD_Fill(0, 0, 128, 128, BLACK);
    LCD_ShowChinese(0, PU_Y_TITLE, (u8 *)"水泵模块", WHITE, BLACK, 16, 0);
    pu_draw_state();
    LCD_ShowString(0, PU_Y_HINT1, (u8 *)"SW:ON/OFF", WHITE, BLACK, 16, 0);
    LCD_ShowString(0, PU_Y_HINT2, (u8 *)"DN:BACK", WHITE, BLACK, 16, 0);
    alarm_manual_override(ALARM_MANUAL_PUMP);
}

void pump_view_tick(void)
{
    /* 状态仅在用户操作时变化，无需周期刷新 */
}

u8 pump_view_on_event(u8 ev, u8 sw)
{
    if (ev == JOY_DOWN) { alarm_manual_release(ALARM_MANUAL_PUMP); return 1; }     /* 下面 -> 返回上级菜单 */

    if (sw) {
        if (pump_is_on()) pump_off();
        else              pump_on();
        pu_draw_state();
        return 0;
    }

    return 0;
}
