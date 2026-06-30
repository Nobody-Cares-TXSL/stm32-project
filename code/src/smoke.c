#include "smoke.h"
#include "io.h"        /* GPIO_init */

/* ====== 引脚配置（换线/换板只改这里） ====== */
#define SMOKE_AO_PORT        GPIOC
#define SMOKE_AO_PIN         GPIO_Pin_0       /* PC0 = ADC1_IN10 */
#define SMOKE_AO_CH          ADC_Channel_10
#define SMOKE_DO_PORT        GPIOB
#define SMOKE_DO_PIN         GPIO_Pin_10      /* PB10 数字输入 */
#define SMOKE_DO_ACTIVE_LOW  1                /* 1=低电平报警(MQ-2板常见)，0=高电平报警 */

/* 注意：ADC1 已由 joystick_init() 初始化（单次转换、软件触发、右对齐、55.5 周期、已校准），
 *       本模块复用 ADC1，不重复初始化，也不启用 ADC2。
 *       smoke_read_adc 与 joystick.c 内部 joy_read_adc 同款，仅 ~5 行重复；
 *       待第三个 ADC 模块（如光敏）落地时再统一抽公共 adc 模块。 */

/* 单次读取 ADC1 指定通道（与 joystick.c 的 joy_read_adc 一致） */
static u16 smoke_read_adc(u8 ch)
{
    ADC_RegularChannelConfig(ADC1, ch, 1, ADC_SampleTime_55Cycles5);
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);
    {
        u16 _to = 0xFFFF;
        while (!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) && --_to);
        if (_to == 0) return 0;  /* ADC timeout */
    }
    return ADC_GetConversionValue(ADC1);
}

void smoke_init(void)
{
    /* AO 口配为模拟输入（AIN），供 ADC 采集 */
    GPIO_init(RCC_APB2Periph_TYPE_C, SMOKE_AO_PIN, GPIO_Mode_AIN, SMOKE_AO_PORT);
    /* DO 口配为上拉输入（IPU），稳定读取数字电平 */
    GPIO_init(RCC_APB2Periph_TYPE_B, SMOKE_DO_PIN, GPIO_Mode_IPU, SMOKE_DO_PORT);
}

/* AO 浓度：4 次采样取平均，返回 0-4095 */
u16 smoke_read_ao(void)
{
    u32 sum = 0;
    u8 i;
    for (i = 0; i < 4; i++) sum += smoke_read_adc(SMOKE_AO_CH);
    return (u16)(sum / 4);
}

/* DO 报警状态：按 SMOKE_DO_ACTIVE_LOW 归一为 1=报警 / 0=安全 */
u8 smoke_read_alarm(void)
{
    u8 bit = GPIO_ReadInputDataBit(SMOKE_DO_PORT, SMOKE_DO_PIN);
#if SMOKE_DO_ACTIVE_LOW
    return (bit == 0) ? 1 : 0;     /* 低电平 = 报警 */
#else
    return (bit != 0) ? 1 : 0;     /* 高电平 = 报警 */
#endif
}
