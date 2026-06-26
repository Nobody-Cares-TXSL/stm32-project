#include "stm32f10x.h"
#include "delay.h"
#include "lcd_ui.h"    /* ui_init */
#include "joystick.h"
#include "menu.h"

int main(void)
{
    SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK_Div8);

    ui_init();          /* LCD 初始化 + 黑底清屏 */
    joystick_init();    /* ADC(X=PB1, Y=PC4) + SW(PA2) */
    menu_init();        /* 绘制初始菜单（光标在第 0 行） */

    ui_login("欢迎进入");
    delay_s(2);

    while (1) {
        u8 ev = joystick_event();
        u8 sw = joystick_pressed();
        if (ev != JOY_MID || sw) {
            menu_handle(ev, sw);   /* 上下移动光标, SW 确认 */
        }
        delay_ms(10);   /* 轮询节流 */
    }
}
