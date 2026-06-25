#ifndef __IO_H
#define __IO_H

#include "stm32f10x.h"

/* ====== 时钟时能位类型 ====== */
#define RCC_APB2Periph_TYPE_A RCC_APB2Periph_GPIOA // GPIOA 的时钟使能位
#define RCC_APB2Periph_TYPE_B RCC_APB2Periph_GPIOB // GPIOB 的时钟使能位
#define RCC_APB2Periph_TYPE_C RCC_APB2Periph_GPIOC // GPIOC 的时钟使能位

/* ====== 引脚类型 ====== */
#define GPIO_PortSource_TYPE_A GPIO_PortSourceGPIOA // GPIOA 引脚
#define GPIO_PortSource_TYPE_C GPIO_PortSourceGPIOC // GPIOC 引脚




void GPIO_init(uint32_t pin_type, uint16_t pin, GPIOMode_TypeDef mode, GPIO_TypeDef* GPIOx);
void EXTI_init(uint8_t port_source, uint8_t pin_source, uint32_t line, EXTITrigger_TypeDef trigger);
void NVIC_init(uint8_t irq_channel, uint8_t preemption_priority, uint8_t sub_priority);
void ADC_init(ADC_TypeDef* ADCx, uint32_t mode, FunctionalState scan, FunctionalState cont, uint32_t trig, uint32_t align, uint8_t nch);

#endif /* __IO_H */
