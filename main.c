#include "stm32f10x.h"
#include "delay.h"
#include "lcd_ui.h"    /* ui_init */
#include "joystick.h"
#include "menu.h"
#include "lcd.h"

int main(void)
{
    SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK_Div8);

    ui_init();          /* LCD init */
    joystick_init();    /* ADC(X=PB1, Y=PC4) + SW(PA2) */
    menu_init();        /* draw initial menu */

    ui_login("»¶Ó­½øÈë");
    delay_s(2);
    LCD_Fill(0, 0, 128, 128, BLACK);

    while (1) {
        u8 ev = joystick_event();
        u8 sw = joystick_pressed();
        if (ev != JOY_MID || sw) {
            menu_handle(ev, sw);
        }
        delay_ms(10);
    }
}
