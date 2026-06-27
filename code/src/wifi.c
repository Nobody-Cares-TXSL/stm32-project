#include "wifi.h"
#include "io.h"        /* GPIO_init / NVIC_init / USART_init */
#include "usart.h"     /* USART1_send_byte（ESP 响应转发到 log） */
#include "delay.h"     /* delay_ms */
#include <stdio.h>     /* printf */

/* ====== USART2 引脚 / 波特率（ESP-01S 接 PA2/PA3） ====== */
#define WIFI_PORT      GPIOA
#define WIFI_TX_PIN    GPIO_Pin_2      /* PA2 = USART2_TX -> ESP RX */
#define WIFI_RX_PIN    GPIO_Pin_3      /* PA3 = USART2_RX <- ESP TX */
#define WIFI_CLK_TYPE  RCC_APB2Periph_TYPE_A
#define WIFI_BAUD      115200          /* ESP-01S 默认；不通则降 9600 */

/* ====== 接收缓冲（中断写、主循环读） ====== */
char WIFI_RX_BUF[WIFI_RX_BUF_LEN];
volatile u8 WIFI_RX_STA = 0;
static u16 wifi_rx_idx = 0;

/* ====== USART2 初始化 ====== */
static void wifi_usart2_init(void)
{
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);   /* USART2 挂 APB1 */
    GPIO_init(WIFI_CLK_TYPE, WIFI_TX_PIN, GPIO_Mode_AF_PP, WIFI_PORT);
    GPIO_init(WIFI_CLK_TYPE, WIFI_RX_PIN, GPIO_Mode_IPU, WIFI_PORT);
    USART_init(USART2, WIFI_BAUD, USART_WordLength_8b, USART_StopBits_1,
               USART_Parity_No, USART_Mode_Rx | USART_Mode_Tx);
    USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
    USART_Cmd(USART2, ENABLE);
    NVIC_init(USART2_IRQn, 1, 1);   /* 低于 USART1(0,0)，log 不被阻塞 */
}

/* ====== 发送（阻塞，逐字节） ====== */
static void wifi_send_str(const char *s)
{
    while (*s) {
        while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
        USART_SendData(USART2, *s++);
    }
}

/* ====== 初始化：USART2 + 等 ESP 启动 + 发 AT 验证通信 ====== */
void wifi_init(void)
{
    wifi_usart2_init();
    delay_ms(3000);                 /* 等 ESP-01S 启动完成 */
    printf("[WIFI] send AT\r\n");
    wifi_send_str("AT\r\n");        /* ESP 应回 OK（见 log 转发） */
}

/* ====== USART2 接收中断：ESP 响应转发到 log + 存缓冲 ====== */
void USART2_IRQHandler(void)
{
    if (USART_GetITStatus(USART2, USART_IT_RXNE) != RESET) {
        char ch = USART_ReceiveData(USART2);
        USART1_send_byte(ch);      /* 转发到 log：ESP 回什么就显示什么 */
        if (wifi_rx_idx < WIFI_RX_BUF_LEN - 1)
            WIFI_RX_BUF[wifi_rx_idx++] = ch;
        if (ch == '\n') {
            WIFI_RX_STA = 1;
            wifi_rx_idx = 0;
        }
        USART_ClearITPendingBit(USART2, USART_IT_RXNE);
    }
}
