#include "pump.h"
#include "io.h"        /* GPIO_init */

/* ====== 硬件配置（换线只改这里） ====== */
#define PUMP_PORT       GPIOA
#define PUMP_PIN        GPIO_Pin_1       /* PA1 → 继电器 IN1 */
#define PUMP_CLK        RCC_APB2Periph_TYPE_A

/* 有效电平：1=高电平继电器吸合(常见)，0=低电平吸合 */
#define PUMP_ACTIVE_HIGH  1

static u8 pump_state = 0;     /* 0=关，1=开 */

void pump_init(void)
{
    GPIO_init(PUMP_CLK, PUMP_PIN, GPIO_Mode_Out_PP, PUMP_PORT);
    pump_off();   /* 默认关闭 */
}

void pump_on(void)
{
    pump_state = 1;
#if PUMP_ACTIVE_HIGH
    GPIO_SetBits(PUMP_PORT, PUMP_PIN);
#else
    GPIO_ResetBits(PUMP_PORT, PUMP_PIN);
#endif
}

void pump_off(void)
{
    pump_state = 0;
#if PUMP_ACTIVE_HIGH
    GPIO_ResetBits(PUMP_PORT, PUMP_PIN);
#else
    GPIO_SetBits(PUMP_PORT, PUMP_PIN);
#endif
}

u8 pump_is_on(void)
{
    return pump_state;
}
