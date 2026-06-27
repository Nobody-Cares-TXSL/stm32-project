#include "usart.h"
#include "io.h"        /* GPIO_init / NVIC_init / USART_init */
#include <stdio.h>     /* fputc 的 FILE */

/* ====== USART1 引脚 / 波特率（换线只改这里） ====== */
#define USART1_PORT      GPIOA
#define USART1_TX_PIN    GPIO_Pin_9      /* PA9 = USART1_TX */
#define USART1_RX_PIN    GPIO_Pin_10     /* PA10 = USART1_RX */
#define USART1_CLK_TYPE  RCC_APB2Periph_TYPE_A
#define USART1_BAUD      9600

/* ====== 接收缓冲（中断写、主循环读） ====== */
char USART1_RX_BUF[USART1_REC_LEN];
volatile u8 USART1_RX_STA = 0;            /* 1 = 一帧就绪 */
static u16 usart1_rx_idx = 0;              /* 当前写入位置（模块内部） */

/* ====== USART1 完整初始化（GPIO + 时钟 + USART_init + NVIC） ====== */
void USART1_init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);   /* USART1 挂 APB2 */
    GPIO_init(USART1_CLK_TYPE, USART1_TX_PIN, GPIO_Mode_AF_PP, USART1_PORT);   /* TX 复用推挽 */
    GPIO_init(USART1_CLK_TYPE, USART1_RX_PIN, GPIO_Mode_IPU, USART1_PORT);     /* RX 上拉输入 */
    USART_init(USART1, USART1_BAUD, USART_WordLength_8b, USART_StopBits_1,
               USART_Parity_No, USART_Mode_Rx | USART_Mode_Tx);
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);   /* 开接收中断 */
    USART_Cmd(USART1, ENABLE);
    NVIC_init(USART1_IRQn, 0, 0);                     /* 抢占 0 子 0（调试通道，最高优先级） */
}

/* ====== 发送 ====== */
void USART1_send_byte(u8 data)
{
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
    USART_SendData(USART1, data);
}

void USART1_send_str(char *p)
{
    while (*p) USART1_send_byte(*p++);
}

/* ====== printf 重定向（需 Keil 勾 Use MicroLIB） ====== */
int fputc(int ch, FILE *f)
{
    while ((USART1->SR & USART_SR_TC) == 0);   /* 等上一字节发完 */
    USART1->DR = (u8)ch;
    return ch;
}

/* ====== USART1 接收中断：按 \n 切帧，缓冲满也算一帧 ====== */
void USART1_IRQHandler(void)
{
    if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET) {
        char ch = USART_ReceiveData(USART1);
        if (usart1_rx_idx < USART1_REC_LEN)
            USART1_RX_BUF[usart1_rx_idx++] = ch;
        if (ch == '\n' || usart1_rx_idx >= USART1_REC_LEN) {
            USART1_RX_STA = 1;        /* 通知主循环：一帧就绪 */
            usart1_rx_idx = 0;         /* 复位，准备下一帧 */
        }
        USART_ClearITPendingBit(USART1, USART_IT_RXNE);
    }
}
