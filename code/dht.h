#ifndef __DHT_H
#define __DHT_H

#include "stm32f10x.h"

/* 读单颗 DHT11（单总线协议）；校验通过返回1，*temp/*humi 为整数值；失败（超时/校验错）返回0 */
u8 dht_read(GPIO_TypeDef *port, uint16_t pin, u8 *temp, u8 *humi);

#endif /* __DHT_H */
