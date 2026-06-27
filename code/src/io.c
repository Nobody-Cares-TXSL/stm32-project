#include "io.h"/* 按 ISR 写 state_dirty / key_event */
#include "delay.h"		/* Get_Adc_Average 用 delay_ms */

/* 初始化结构体（本文件内使用，封装为 static） */
static GPIO_InitTypeDef GPIO_InitStructure;
static EXTI_InitTypeDef EXTI_InitStruct;
static NVIC_InitTypeDef NVIC_InitStruct;
static ADC_InitTypeDef ADC_InitStruct;
static TIM_TimeBaseInitTypeDef TIM_TB_InitStruct;
static TIM_OCInitTypeDef TIM_OC_InitStruct;
static USART_InitTypeDef USART_InitStruct;

// 初始化 GPIO
void GPIO_init(uint32_t pin_type, uint16_t pin, GPIOMode_TypeDef mode, GPIO_TypeDef* GPIOx)
{
	RCC_APB2PeriphClockCmd(pin_type, ENABLE);
	GPIO_InitStructure.GPIO_Pin = pin;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = mode;
	GPIO_Init(GPIOx, &GPIO_InitStructure);
}

// 初始化 EXTI
void EXTI_init(uint8_t port_source, uint8_t pin_source, uint32_t line, EXTITrigger_TypeDef trigger)
{
	GPIO_EXTILineConfig(port_source, pin_source);
	EXTI_InitStruct.EXTI_Line = line;
	EXTI_InitStruct.EXTI_LineCmd = ENABLE;
	EXTI_InitStruct.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStruct.EXTI_Trigger = trigger;
	EXTI_Init(&EXTI_InitStruct);
}

// 初始化 NVIC
void NVIC_init(uint8_t irq_channel, uint8_t preemption_priority, uint8_t sub_priority)
{
	NVIC_InitStruct.NVIC_IRQChannel = irq_channel;
	NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
	NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = preemption_priority;
	NVIC_InitStruct.NVIC_IRQChannelSubPriority = sub_priority;
	NVIC_Init(&NVIC_InitStruct);
}

// 初始化 ADC
void ADC_init(ADC_TypeDef* ADCx, uint32_t mode, FunctionalState scan, FunctionalState cont, uint32_t trig, uint32_t align, uint8_t nch)
{
	ADC_DeInit(ADCx);
	ADC_InitStruct.ADC_Mode = mode;
	ADC_InitStruct.ADC_ScanConvMode = scan;
	ADC_InitStruct.ADC_ContinuousConvMode = cont;
	ADC_InitStruct.ADC_ExternalTrigConv = trig;
	ADC_InitStruct.ADC_DataAlign = align;
	ADC_InitStruct.ADC_NbrOfChannel = nch;
	ADC_Init(ADCx, &ADC_InitStruct);
}

// 初始化 TIM 时基（调用前需自行使能 RCC 时钟）
void TIM_base_init(TIM_TypeDef* TIMx, uint16_t psc, uint16_t arr)
{
	TIM_TB_InitStruct.TIM_Prescaler = psc;
	TIM_TB_InitStruct.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TB_InitStruct.TIM_Period = arr;
	TIM_TB_InitStruct.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseInit(TIMx, &TIM_TB_InitStruct);
	TIM_ARRPreloadConfig(TIMx, ENABLE);
}

// 初始化 TIM PWM 输出通道（内部启用 TIM_Cmd）
void TIM_pwm_init(TIM_TypeDef* TIMx, uint16_t channel, uint16_t pulse, uint8_t polarity_high)
{
	TIM_OC_InitStruct.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OC_InitStruct.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OC_InitStruct.TIM_Pulse = pulse;
	TIM_OC_InitStruct.TIM_OCPolarity = polarity_high ? TIM_OCPolarity_High : TIM_OCPolarity_Low;
	switch (channel) {
		case 1: TIM_OC1Init(TIMx, &TIM_OC_InitStruct); TIM_OC1PreloadConfig(TIMx, TIM_OCPreload_Enable); break;
		case 2: TIM_OC2Init(TIMx, &TIM_OC_InitStruct); TIM_OC2PreloadConfig(TIMx, TIM_OCPreload_Enable); break;
		case 3: TIM_OC3Init(TIMx, &TIM_OC_InitStruct); TIM_OC3PreloadConfig(TIMx, TIM_OCPreload_Enable); break;
		case 4: TIM_OC4Init(TIMx, &TIM_OC_InitStruct); TIM_OC4PreloadConfig(TIMx, TIM_OCPreload_Enable); break;
	}
	TIM_Cmd(TIMx, ENABLE);
}

// 初始化 USART 配置（波特率/字长/停止位/校验/模式；GPIO/时钟/NVIC 由调用方处理）
void USART_init(USART_TypeDef* USARTx, uint32_t baud, uint16_t word_length, uint16_t stop_bits, uint16_t parity, uint16_t mode)
{
	USART_InitStruct.USART_BaudRate = baud;
	USART_InitStruct.USART_WordLength = word_length;
	USART_InitStruct.USART_StopBits = stop_bits;
	USART_InitStruct.USART_Parity = parity;
	USART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStruct.USART_Mode = mode;
	USART_Init(USARTx, &USART_InitStruct);
}
