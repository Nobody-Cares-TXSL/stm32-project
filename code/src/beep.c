#include "beep.h"
#include "io.h"

/* ====== pin config ====== */
#define BEEP_PORT    GPIOA
#define BEEP_PIN     GPIO_Pin_4       /* PA4 -> buzzer */
#define BEEP_CLK     RCC_APB2Periph_TYPE_A

/* active low: pin=LOW -> beep on (common buzzer module) */
#define BEEP_ON_LEVEL   0
#define BEEP_OFF_LEVEL  1

static u8 beep_state = 0;

void beep_init(void)
{
    GPIO_init(BEEP_CLK, BEEP_PIN, GPIO_Mode_Out_PP, BEEP_PORT);
    beep_off();
}

void beep_on(void)
{
    GPIO_WriteBit(BEEP_PORT, BEEP_PIN, (BitAction)BEEP_ON_LEVEL);
    beep_state = 1;
}

void beep_off(void)
{
    GPIO_WriteBit(BEEP_PORT, BEEP_PIN, (BitAction)BEEP_OFF_LEVEL);
    beep_state = 0;
}

void beep_toggle(void)
{
    if (beep_state) beep_off();
    else            beep_on();
}

u8 beep_get_state(void)
{
    return beep_state;
}
