#include "beep_view.h"
#include "beep.h"
#include "lcd.h"
#include "joystick.h"

/* ====== layout (128x128, 16px/font) ====== */
#define BP_Y_TITLE  0      /* title */
#define BP_Y_STATE  28      /* ON/OFF state */
#define BP_Y_HINT1  84      /* hint line 1 */
#define BP_Y_HINT2  100      /* hint line 2 */

static void bp_draw_state(u8 on)
{
    LCD_Fill(0, BP_Y_STATE, 128, BP_Y_STATE + 16, BLACK);
    LCD_ShowString(0, BP_Y_STATE, (u8 *)"BEEP:", WHITE, BLACK, 16, 0);
    if (on) LCD_ShowString(40, BP_Y_STATE, (u8 *)"ON", GREEN, BLACK, 16, 0);
    else    LCD_ShowString(40, BP_Y_STATE, (u8 *)"OFF", RED, BLACK, 16, 0);
}

void beep_view_enter(void)
{
    LCD_Fill(0, 0, 128, 128, BLACK);
    LCD_ShowChinese(0, BP_Y_TITLE, (u8 *)"\xB7\xE4\xC3\xF9\xC6\xF7\xC4\xA3\xBF\xE9",
                     WHITE, BLACK, 16, 0);   /* "·äÃùÆ÷Ä£¿é" GB2312 */
    bp_draw_state(beep_get_state());
    LCD_ShowString(0, BP_Y_HINT1, (u8 *)"SW:ON/OFF", WHITE, BLACK, 16, 0);
    LCD_ShowString(0, BP_Y_HINT2, (u8 *)"DN:BACK", WHITE, BLACK, 16, 0);
}

void beep_view_tick(void)
{
    /* actuator: state changes only on user input, no refresh needed */
}

u8 beep_view_on_event(u8 ev, u8 sw)
{
    if (ev == JOY_DOWN) return 1;     /* DOWN -> back to menu */

    if (sw) {
        beep_toggle();
        bp_draw_state(beep_get_state());
        return 0;
    }

    return 0;
}
