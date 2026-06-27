#include "light.h"
#include "io.h"        /* GPIO_init */

/* ====== 硬件配置（换线/换板只改这里） ====== */
#define LIGHT_AO_PORT        GPIOC
#define LIGHT_AO_PIN         GPIO_Pin_3       /* PC3 = ADC1_IN13 */
#define LIGHT_AO_CH          ADC_Channel_13
#define LIGHT_DO_PORT        GPIOB
#define LIGHT_DO_PIN         GPIO_Pin_13      /* PB13 数字输入 */
#define LIGHT_DO_ACTIVE_LOW  1                /* 1=低电平触发(常见)，0=高电平触发 */

/* ADC1 由 joystick_init() 初始化（单次转换、软件触发、右对齐、55.5 周期、已校准）。
 * 本模块复用 ADC1，不重复初始化。light_read_adc 从 joystick.c 内部函数复制，~5 行重复。
 * 当前项目已有 joystick/smoke/light 三个 ADC 模块，后续应统一抽公共 adc.c。 */

static u16 light_read_adc(u8 ch)
{
    ADC_RegularChannelConfig(ADC1, ch, 1, ADC_SampleTime_55Cycles5);
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);
    while (!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC));
    return ADC_GetConversionValue(ADC1);
}

void light_init(void)
{
    /* AO 配置为模拟输入（AIN），由 ADC 采集 */
    GPIO_init(RCC_APB2Periph_TYPE_C, LIGHT_AO_PIN, GPIO_Mode_AIN, LIGHT_AO_PORT);
    /* DO 配置为数字输入（IPU），稳定读取数字电平 */
    GPIO_init(RCC_APB2Periph_TYPE_B, LIGHT_DO_PIN, GPIO_Mode_IPU, LIGHT_DO_PORT);
}

/* AO 亮度：4 次测量取平均，返回 0-4095（值越大越亮） */
u16 light_read_ao(void)
{
    u32 sum = 0;
    u8 i;
    for (i = 0; i < 4; i++) sum += light_read_adc(LIGHT_AO_CH);
    return (u16)(sum / 4);
}

/* DO 阈值状态：按 LIGHT_DO_ACTIVE_LOW 归一为 1=触发 / 0=正常 */
u8 light_read_alarm(void)
{
    u8 bit = GPIO_ReadInputDataBit(LIGHT_DO_PORT, LIGHT_DO_PIN);
#if LIGHT_DO_ACTIVE_LOW
    return (bit == 0) ? 1 : 0;     /* 低电平 = 触发 */
#else
    return (bit != 0) ? 1 : 0;     /* 高电平 = 触发 */
#endif
}
