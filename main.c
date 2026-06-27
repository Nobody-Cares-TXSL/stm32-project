#include "stm32f10x.h"
#include "delay.h"
#include "lcd_ui.h"    /* ui_init */
#include "joystick.h"
#include "smoke.h"
#include "fan.h"     /* fan_init */
#include "light.h"   /* light_init */
#include "flame.h"   /* flame_init */
#include "pump.h"    /* pump_init */
#include "ledstrip.h" /* ledstrip_init */
#include "beep.h"    /* beep_init */
#include "usart.h"   /* USART1 调试 log */
#include "wifi.h"    /* ESP-01S WiFi */
#include "menu.h"
#include "lcd.h"

int main(void)
{
    SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK_Div8);
    USART1_init();      /* 调试串口先开，便于后续 printf */
    printf("system start");   /* 上电验证：串口助手应收到此句 */

    ui_init();          /* LCD init */
    joystick_init();    /* ADC(X=PB1, Y=PC4) + SW(PA2) */
    smoke_init();    /* 烟雾：PC0(AO) + PB10(DO)；ADC1 由 joystick_init 已就绪 */
    fan_init();    /* fan: PB0(TIM3_CH3 PWM) */
    light_init();  /* light: PC3(AO) + PB13(DO); ADC1 ready */

    flame_init();   /* flame: PC1/PC2(AO) + PB11/PB12(DO); ADC1 ready */
    pump_init();   /* pump: PA1 GPIO -> relay IN1 */
    ledstrip_init(); /* ledstrip: PC12(DIN1) + PC11(DIN2) WS2812B */
    beep_init();   /* beep: PA4 GPIO */
    wifi_init();     /* USART2 + ESP-01S（v1: 发 AT 验证通信） */
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
