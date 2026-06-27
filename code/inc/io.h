#ifndef __IO_H
#define __IO_H

#include "stm32f10x.h"

/* ====== 时钟使能位定义 ====== */
#define RCC_APB2Periph_TYPE_A RCC_APB2Periph_GPIOA // GPIOA 时钟使能位
#define RCC_APB2Periph_TYPE_B RCC_APB2Periph_GPIOB // GPIOB 时钟使能位
#define RCC_APB2Periph_TYPE_C RCC_APB2Periph_GPIOC // GPIOC 时钟使能位

/* ====== 端口源定义 ====== */
#define GPIO_PortSource_TYPE_A GPIO_PortSourceGPIOA // GPIOA 端口
#define GPIO_PortSource_TYPE_C GPIO_PortSourceGPIOC // GPIOC 端口


/* ====== GPIO ====== */
void GPIO_init(uint32_t pin_type, uint16_t pin, GPIOMode_TypeDef mode, GPIO_TypeDef* GPIOx);

/* ====== EXTI ====== */
void EXTI_init(uint8_t port_source, uint8_t pin_source, uint32_t line, EXTITrigger_TypeDef trigger);

/* ====== NVIC ====== */
void NVIC_init(uint8_t irq_channel, uint8_t preemption_priority, uint8_t sub_priority);

/* ====== ADC ====== */
void ADC_init(ADC_TypeDef* ADCx, uint32_t mode, FunctionalState scan, FunctionalState cont, uint32_t trig, uint32_t align, uint8_t nch);

/* ====== TIM ====== */
void TIM_base_init(TIM_TypeDef* TIMx, uint16_t psc, uint16_t arr);
void TIM_pwm_init(TIM_TypeDef* TIMx, uint16_t channel, uint16_t pulse, uint8_t polarity_high);

/* ====== 串口 ====== */
void USART_init(USART_TypeDef* USARTx, uint32_t baud, uint16_t word_length, uint16_t stop_bits, uint16_t parity, uint16_t mode);

#endif /* __IO_H */
