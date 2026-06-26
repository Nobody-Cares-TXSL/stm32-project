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

    ui_login("欢迎进入");
    delay_s(2);
    LCD_Fill(0, 0, 128, 128, BLACK);

    menu_init();        /* draw initial menu */

    while (1) {
        u8 ev = joystick_event();
        u8 sw = joystick_pressed();
        menu_handle(ev, sw);   /* 每轮调用：主菜单按需重绘，视图态周期 tick */
        delay_ms(10);
    }
}
