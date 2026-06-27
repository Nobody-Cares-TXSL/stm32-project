#include "io.h"/* 按键 ISR 写 state_dirty / key_event */
#include "delay.h"		/* Get_Adc_Average 用 delay_ms */

/* 初始化结构体（仅本文件使用，封装为 static） */
static GPIO_InitTypeDef GPIO_InitStructure;
static EXTI_InitTypeDef EXTI_InitStruct;
static NVIC_InitTypeDef NVIC_InitStruct;
static ADC_InitTypeDef ADC_InitStruct;

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

