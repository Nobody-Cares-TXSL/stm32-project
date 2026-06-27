#ifndef __WIFI_H
#define __WIFI_H

#include "stm32f10x.h"

#define WIFI_RX_BUF_LEN  128   /* ESP 响应接收缓冲 */

extern char WIFI_RX_BUF[WIFI_RX_BUF_LEN];   /* 接收缓冲 */
extern volatile u8 WIFI_RX_STA;              /* 1 = 收到一帧（\n 结尾） */

void wifi_init(void);   /* USART2 初始化 + 等 ESP 启动 + 发 AT（v1：验证通信） */

#endif /* __WIFI_H */
