#include "fan_view.h"
#include "alarm.h"
#include "fan.h"       /* fan_set_speed / fan_get_speed */
#include "lcd.h"       /* LCD_Fill / LCD_ShowChinese / LCD_ShowString + BLACK/WHITE/RED/GREEN */
#include "joystick.h"  /* JOY_DOWN / JOY_LEFT / JOY_RIGHT */
#include <stdio.h>     /* sprintf */

/* ====== 布局（128x128，16px/行） ====== */
#define FN_Y_TITLE   0      /* 标题 */
#define FN_Y_STATE  28      /* FAN ON/OFF 状态 */
#define FN_Y_PWM    56      /* PWM 占空比 */
#define FN_Y_HINT1  84      /* 操作提示1 */
#define FN_Y_HINT2 100      /* 操作提示2 */
#define FN_Y_HINT3 110      /* 返回提示 */

/* ====== 调速步进 ====== */
#define FAN_SPEED_STEP  10   /* 每次按键调整 10% */

/* 状态行绘制 */
static void fn_draw_state(u8 speed)
{
    LCD_Fill(0, FN_Y_STATE, 128, FN_Y_STATE + 16, BLACK);
    LCD_ShowString(0, FN_Y_STATE, (u8 *)"FAN:", WHITE, BLACK, 16, 0);
    if (speed > 0) LCD_ShowString(32, FN_Y_STATE, (u8 *)"ON", GREEN, BLACK, 16, 0);
    else           LCD_ShowString(32, FN_Y_STATE, (u8 *)"OFF", RED, BLACK, 16, 0);
}

/* PWM 行绘制 */
static void fn_draw_pwm(u8 speed)
{
    char buf[8];
    LCD_Fill(0, FN_Y_PWM, 128, FN_Y_PWM + 16, BLACK);
    LCD_ShowString(0, FN_Y_PWM, (u8 *)"PWM:", WHITE, BLACK, 16, 0);
    sprintf(buf, "%d%%", speed);
    LCD_ShowString(32, FN_Y_PWM, (u8 *)buf, WHITE, BLACK, 16, 0);
}

void fan_view_enter(void)
{
    LCD_Fill(0, 0, 128, 128, BLACK);
    LCD_ShowChinese(0, FN_Y_TITLE, (u8 *)"风扇模块", WHITE, BLACK, 16, 0);
    fn_draw_state(fan_get_speed());
    fn_draw_pwm(fan_get_speed());
    LCD_ShowString(0, FN_Y_HINT1, (u8 *)"SW:ON/OFF", WHITE, BLACK, 16, 0);
    LCD_ShowString(0, FN_Y_HINT2, (u8 *)"L/R:SPEED", WHITE, BLACK, 16, 0);
    LCD_ShowString(0, FN_Y_HINT3, (u8 *)"DN:BACK", WHITE, BLACK, 16, 0);
    alarm_manual_override(ALARM_MANUAL_FAN);
}

void fan_view_tick(void)
{
    /* 风扇状态仅在用户操作时变化，无需周期刷新 */
}

u8 fan_view_on_event(u8 ev, u8 sw)
{
    u8 speed;

    if (ev == JOY_DOWN) { alarm_manual_release(ALARM_MANUAL_FAN); return 1; }     /* 下拨 -> 返回主菜单 */

    if (sw) {
        /* SW 按下：ON/OFF 切换 */
        speed = fan_get_speed();
        if (speed > 0) fan_set_speed(0);
        else           fan_set_speed(FAN_SPEED_STEP);   /* 默认 10% 启动 */
        fn_draw_state(fan_get_speed());
        fn_draw_pwm(fan_get_speed());
        return 0;
    }

    if (ev == JOY_RIGHT || ev == JOY_UP) {
        speed = fan_get_speed();
        if (speed > 0) {   /* 仅在开启状态调速 */
            speed = (speed + FAN_SPEED_STEP > 100) ? 100 : speed + FAN_SPEED_STEP;
            fan_set_speed(speed);
            fn_draw_state(speed);
            fn_draw_pwm(speed);
        }
        return 0;
    }

    if (ev == JOY_LEFT) {
        speed = fan_get_speed();
        if (speed > 0) {   /* 仅在开启状态调速 */
            speed = (speed < FAN_SPEED_STEP) ? 0 : speed - FAN_SPEED_STEP;
            fan_set_speed(speed);
            fn_draw_state(speed);
            fn_draw_pwm(speed);
        }
        return 0;
    }

    return 0;
}
