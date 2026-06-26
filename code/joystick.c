#include "joystick.h"
#include "io.h"        /* GPIO_init / ADC_init 初始化封装 */

/* ====== 引脚 / 通道 / 阈值（换线只改这里） ====== */
/* X 轴 = PB1, Y 轴 = PC4, SW(按下) = PA2 */
#define JOY_X_PIN    GPIO_Pin_1     /* PB1 */
#define JOY_Y_PIN    GPIO_Pin_4     /* PC4 */
#define JOY_SW_PIN   GPIO_Pin_2     /* PA2, 上拉, 按下为低 */
#define JOY_X_CH     ADC_Channel_9  /* PB1 = ADC 通道 9  */
#define JOY_Y_CH     ADC_Channel_14 /* PC4 = ADC 通道 14 */
#define JOY_MID_MIN  1500           /* 死区下限（现场可调） */
#define JOY_MID_MAX  3400           /* 死区上限 */

/* SW 消抖：连续多少次一致才确认稳定态 */
#define SW_DEBOUNCE  5

static u8 joy_last_dir = JOY_MID;   /* 边沿记忆：仅 中点→方向 才触发 */

/* 软件触发读一次 ADC 指定通道 */
/**
 * @brief 读取摇杆的ADC转换值
 * @note 该函数通过软件触发ADC转换，并阻塞等待转换完成后返回结果
 * @param ch 需要读取的ADC通道号（8位无符号整数）
 * @return 返回指定通道的ADC转换值（16位无符号整数）
 */
static u16 joy_read_adc(u8 ch)
{
    /* 配置ADC1的规则组通道，设置转换顺序为1，采样时间为55.5个时钟周期 */
    ADC_RegularChannelConfig(ADC1, ch, 1, ADC_SampleTime_55Cycles5);

    /* 使能ADC1的软件转换启动功能，开始进行ADC转换 */
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);

    /* 阻塞等待ADC1转换完成，直到获取到结束标志（EOC） */
    while (!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC));

    // 读取并返回ADC1的转换数值
    return ADC_GetConversionValue(ADC1);
}

/* 多次采样取平均，减少ADC噪声 */
static u16 joy_read_adc_avg(u8 ch, u8 times)
{
    u32 sum = 0;
    u8 i;
    for (i = 0; i < times; i++) {
        sum += joy_read_adc(ch);
    }
    return (u16)(sum / times);
}

/**
 * @brief  初始化摇杆模块（Joystick）
 * @note   配置摇杆的X轴和Y轴使用ADC采集模拟量，SW按键使用GPIO读取数字量
 * @param  无
 * @retval 无
 */
void joystick_init(void)
{
    /*------------------- 时钟配置部分 -------------------*/
    // 开启 ADC1 的外设时钟（挂载在 APB2 总线上）
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
    
    // 配置 ADC 的时钟分频为 PCLK2 的 4 分频（例如 PCLK2 为 72MHz，则 ADC 时钟为 18MHz，满足 ADC 最大 14MHz 的要求）
    RCC_ADCCLKConfig(RCC_PCLK2_Div4);

    /*------------------- GPIO 配置部分 -------------------*/
    // 初始化摇杆 X 轴引脚：配置为模拟输入模式（AIN），以便 ADC 采集电压信号
    GPIO_init(RCC_APB2Periph_TYPE_B, JOY_X_PIN, GPIO_Mode_AIN, GPIOB);
    
    // 初始化摇杆 Y 轴引脚：配置为模拟输入模式（AIN），以便 ADC 采集电压信号
    GPIO_init(RCC_APB2Periph_TYPE_C, JOY_Y_PIN, GPIO_Mode_AIN, GPIOC);
    
    // 初始化摇杆 SW 按键引脚：配置为上拉输入模式（IPU），未按下时读取高电平，按下时读取低电平
    GPIO_init(RCC_APB2Periph_TYPE_A, JOY_SW_PIN, GPIO_Mode_IPU, GPIOA);

    /*------------------- ADC 配置部分 -------------------*/
    // 初始化 ADC1 参数：
    // ADC_Mode_Independent      : 独立工作模式（非双ADC模式）
    // DISABLE                   : 不开启扫描模式（仅单通道采集）
    // ENABLE                    : 开启连续转换模式（ADC 会持续不断地进行转换）
    // ADC_ExternalTrigConv_None : 不使用外部触发转换（由软件内部触发）
    // ADC_DataAlign_Right       : 转换结果数据右对齐
    // 1                         : 转换通道数为 1
    ADC_init(ADC1, ADC_Mode_Independent, DISABLE, DISABLE,
             ADC_ExternalTrigConv_None, ADC_DataAlign_Right, 1);
             
    // 使能 ADC1 外设
    ADC_Cmd(ADC1, ENABLE);
    
    /*------------------- ADC 校准部分 -------------------*/
    // STM32 的 ADC 在首次使用前建议进行校准，以提高转换精度
    
    // 开启 ADC1 的复位校准
    ADC_ResetCalibration(ADC1);
    // 等待复位校准完成（死循环等待标志位被硬件清零）
    while (ADC_GetResetCalibrationStatus(ADC1));
    
    // 开启 ADC1 的常规校准
    ADC_StartCalibration(ADC1);
    // 等待常规校准完成（死循环等待标志位被硬件清零）
    while (ADC_GetCalibrationStatus(ADC1));
}


/* 读取摇杆瞬时方向：Y 优先(上下), Y 在死区再看 X(左右) */
/* DEBUG: return raw ADC X/Y values for diagnosis */
void joystick_debug_adc(u16 *x, u16 *y)
{
    if (x) *x = joy_read_adc_avg(JOY_X_CH, 4);
    if (y) *y = joy_read_adc_avg(JOY_Y_CH, 4);
}

static u8 joystick_dir(void)
{
    u16 x = joy_read_adc_avg(JOY_X_CH, 4);
    u16 y = joy_read_adc_avg(JOY_Y_CH, 4);
    if (y > JOY_MID_MAX) return JOY_UP;     /* 上 = Y 大 */
    if (y < JOY_MID_MIN) return JOY_DOWN;   /* 下 = Y 小 */
    if (x > JOY_MID_MAX) return JOY_RIGHT;  /* 右 = X 大 */
    if (x < JOY_MID_MIN) return JOY_LEFT;   /* 左 = X 小 */
    return JOY_MID;
}

/* 摇杆方向边沿事件：仅从中点推出时返回方向码，否则返回 JOY_MID */
u8 joystick_event(void)
{
    u8 cur = joystick_dir();
    u8 evt = JOY_MID;
    if (cur != JOY_MID && joy_last_dir == JOY_MID) {
        evt = cur;
    }
    joy_last_dir = cur;
    return evt;
}

/* SW 按下边沿事件：非阻塞计数消抖, 稳定到按下时返回 1 */
u8 joystick_pressed(void)
{
    static u8 sw_stable = 1;   /* 消抖后稳定态, 松开 = 1 */
    static u8 debounce  = 0;
    u8 raw = GPIO_ReadInputDataBit(GPIOA, JOY_SW_PIN);
    u8 evt = 0;
    if (raw != sw_stable) {
        if (++debounce >= SW_DEBOUNCE) {
            sw_stable = raw;
            debounce  = 0;
            if (sw_stable == 0) evt = 1;   /* 刚稳定到按下 */
        }
    } else {
        debounce = 0;
    }
    return evt;
}
