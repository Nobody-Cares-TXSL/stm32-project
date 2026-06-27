#include "flame.h"
#include "io.h"        /* GPIO_init */

/* ====== 传感器 1 引脚（PC1/AO, PB11/DO） ====== */
#define F1_AO_PORT  GPIOC
#define F1_AO_PIN   GPIO_Pin_1       /* PC1 = ADC1_IN11 */
#define F1_AO_CH    ADC_Channel_11
#define F1_DO_PORT  GPIOB
#define F1_DO_PIN   GPIO_Pin_11      /* PB11 */

/* ====== 传感器 2 引脚（PC2/AO, PB12/DO） ====== */
#define F2_AO_PORT  GPIOC
#define F2_AO_PIN   GPIO_Pin_2       /* PC2 = ADC1_IN12 */
#define F2_AO_CH    ADC_Channel_12
#define F2_DO_PORT  GPIOB
#define F2_DO_PIN   GPIO_Pin_12      /* PB12 */

/* 有效电平：1=低电平触发（常见），0=高电平触发 */
#define FLAME_DO_ACTIVE_LOW  1

/* ADC1 由 joystick_init() 初始化（单次转换、软件触发、右对齐、55.5 周期、已校准）。
 * 本项目现有 5 个模块复用 ADC1（joystick/smoke/light/flame×2），后续应统一抽公共 adc.c。 */

static u16 flame_read_adc(u8 ch)
{
    ADC_RegularChannelConfig(ADC1, ch, 1, ADC_SampleTime_55Cycles5);
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);
    while (!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC));
    return ADC_GetConversionValue(ADC1);
}

void flame_init(void)
{
    /* 传感器 1 */
    GPIO_init(RCC_APB2Periph_TYPE_C, F1_AO_PIN, GPIO_Mode_AIN, F1_AO_PORT);
    GPIO_init(RCC_APB2Periph_TYPE_B, F1_DO_PIN, GPIO_Mode_IPU, F1_DO_PORT);
    /* 传感器 2 */
    GPIO_init(RCC_APB2Periph_TYPE_C, F2_AO_PIN, GPIO_Mode_AIN, F2_AO_PORT);
    GPIO_init(RCC_APB2Periph_TYPE_B, F2_DO_PIN, GPIO_Mode_IPU, F2_DO_PORT);
}

u16 flame_read_ao(u8 sensor)
{
    u8 ch = (sensor == FLAME_SENSOR_1) ? F1_AO_CH : F2_AO_CH;
    u32 sum = 0;
    u8 i;
    for (i = 0; i < 4; i++) sum += flame_read_adc(ch);
    return (u16)(sum / 4);
}

u8 flame_read_alarm(u8 sensor)
{
    u8 bit;
    if (sensor == FLAME_SENSOR_1)
        bit = GPIO_ReadInputDataBit(F1_DO_PORT, F1_DO_PIN);
    else
        bit = GPIO_ReadInputDataBit(F2_DO_PORT, F2_DO_PIN);
#if FLAME_DO_ACTIVE_LOW
    return (bit == 0) ? 1 : 0;     /* 低电平 = 火警 */
#else
    return (bit != 0) ? 1 : 0;     /* 高电平 = 火警 */
#endif
}
