#ifndef __USART_H
#define __USART_H

#include "stm32f10x.h"

/* ====== 配置 ====== */
#define USART1_REC_LEN  100   /* 接收缓冲长度（字节） */

/* ====== 对外变量（中断写、主循环读） ====== */
extern char USART1_RX_BUF[USART1_REC_LEN];   /* 接收缓冲 */
extern volatile u8 USART1_RX_STA;             /* 1 = 收到完整一帧（\n 结尾或缓冲满） */

/* ====== 接口 ====== */
void USART1_init(void);        /* 初始化 USART1：9600/8N1，PA9(TX)/PA10(RX)，开 RX 中断，重定向 printf */
void USART1_send_byte(u8 data);
void USART1_send_str(char *p);

#endif /* __USART_H */
