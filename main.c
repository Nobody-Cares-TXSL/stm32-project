#include "stm32f10x.h"
#include "delay.h"
#include "lcd_ui.h"    /* ui_init */
#include "joystick.h"
#include "menu.h"
#include "lcd.h"       /* LCD_Fill, LCD_ShowIntNum, WHITE, BLACK */
#include <stdio.h>      /* sprintf */

/* DEBUG: show raw ADC X/Y values on LCD */
int main(void)
{
    SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK_Div8);

    ui_init();
    joystick_init();

    while (1) {
        u16 ax = 0, ay = 0;
        char buf[24];

        joystick_debug_adc(&ax, &ay);

        LCD_Fill(0, 0, 128, 128, BLACK);
        LCD_ShowString(0, 0, (u8 *)"ADC DEBUG", WHITE, BLACK, 16, 1);
        sprintf(buf, "X:%d", ax);
        LCD_ShowString(0, 20, (u8 *)buf, WHITE, BLACK, 16, 1);
        sprintf(buf, "Y:%d", ay);
        LCD_ShowString(0, 40, (u8 *)buf, WHITE, BLACK, 16, 1);

        delay_ms(200);
    }
}
