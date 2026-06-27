#include "fan.h"
#include "io.h"        /* GPIO_init / TIM_base_init / TIM_pwm_init */

/* ====== 硬件配置（换线/换板只改这里） ====== */
#define FAN_PORT        GPIOB
#define FAN_PIN         GPIO_Pin_0       /* PB0 = TIM3_CH3 */
#define FAN_GPIO_CLK    RCC_APB2Periph_TYPE_B
#define FAN_TIM         TIM3
#define FAN_TIM_RCC     RCC_APB1Periph_TIM3
#define FAN_TIM_CH      3

/* 有效电平：1=高电平转动(NPN/MOSFET)，0=低电平转动(PNP) */
#define FAN_POLARITY_HIGH  1

/* PWM 参数：72MHz / (71+1) / (99+1) = 10kHz */
#define FAN_PWM_PSC     71
#define FAN_PWM_ARR     99

static u8 fan_speed = 0;     /* 当前占空比 0-100 */

void fan_init(void)
{
    RCC_APB1PeriphClockCmd(FAN_TIM_RCC, ENABLE);
    GPIO_init(FAN_GPIO_CLK, FAN_PIN, GPIO_Mode_AF_PP, FAN_PORT);
    TIM_base_init(FAN_TIM, FAN_PWM_PSC, FAN_PWM_ARR);
    TIM_pwm_init(FAN_TIM, FAN_TIM_CH, 0, FAN_POLARITY_HIGH);
}

void fan_set_speed(u8 pct)
{
    u16 ccr;
    if (pct > 100) pct = 100;
    fan_speed = pct;
#if FAN_POLARITY_HIGH
    ccr = (u16)((u32)pct * FAN_PWM_ARR / 100);
#else
    ccr = (u16)((u32)(100 - pct) * FAN_PWM_ARR / 100);
#endif
    TIM_SetCompare3(FAN_TIM, ccr);
}

u8 fan_get_speed(void)
{
    return fan_speed;
}
