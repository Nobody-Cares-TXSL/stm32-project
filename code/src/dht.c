#include "dht.h"
#include "io.h"      /* GPIO_init */
#include "delay.h"   /* delay_us / delay_ms */

/* ====== 内部参数 ====== */
/* 等待超时（循环计数上限）。最长脉冲约80us，留足余量覆盖抖动 */
#define DHT_TIMEOUT  10000
/* 判0/1的门限延时：'0'高电平~27us、'1'高电平~70us，延时40us居中判定 */
#define DHT_BIT_US   40

/* 由端口推导 APB2 时钟使能位（GPIOA/B/C） */
static uint32_t dht_port_clk(GPIO_TypeDef *port)
{
    if (port == GPIOA) return RCC_APB2Periph_GPIOA;
    if (port == GPIOB) return RCC_APB2Periph_GPIOB;
    return RCC_APB2Periph_GPIOC;
}

/* 等待引脚电平变为 level(0/1)；超时返回1（失败），正常返回0 */
static u8 dht_wait(GPIO_TypeDef *port, uint16_t pin, u8 level)
{
    u16 t = 0;
    while (GPIO_ReadInputDataBit(port, pin) != level) {
        if (++t > DHT_TIMEOUT) return 1;
    }
    return 0;
}

/**
 * @brief  读单颗 DHT11
 * @note   时序：主机拉低≥18ms起始 → 切输入读应答(80us低+80us高) → 读40位 → 校验
 *         读取过程阻塞约20~25ms，调用方应节流（约1次/秒）
 * @param  port 传感器所在 GPIO 端口（如 GPIOB）
 * @param  pin  传感器数据线引脚（如 GPIO_Pin_14）
 * @param  temp 输出温度整数（校验通过才写入）
 * @param  humi 输出湿度整数（校验通过才写入）
 * @retval 1=成功，0=失败（超时或校验错，传感器可能未接）
 */
u8 dht_read(GPIO_TypeDef *port, uint16_t pin, u8 *temp, u8 *humi)
{
    u8 buf[5] = {0};
    u8 i, j;
    uint32_t clk = dht_port_clk(port);

    /* 1) 主机起始信号：拉低≥18ms，再拉高保持20~40us */
    GPIO_init(clk, pin, GPIO_Mode_Out_PP, port);
    GPIO_ResetBits(port, pin);
    delay_ms(20);
    GPIO_SetBits(port, pin);
    delay_us(30);

    /* 2) 切上拉输入，等 DHT11 响应（~80us低 + ~80us高） */
    GPIO_init(clk, pin, GPIO_Mode_IPU, port);
    if (dht_wait(port, pin, 0)) return 0;   /* 响应低电平开始 */
    if (dht_wait(port, pin, 1)) return 0;   /* 响应高电平开始 */
    if (dht_wait(port, pin, 0)) return 0;   /* 响应高结束，进入第1位的50us低 */

    /* 3) 读 5 字节 = 40 位（每位：50us低 + 27us高=0 / 70us高=1） */
    for (i = 0; i < 5; i++) {
        u8 val = 0;
        for (j = 0; j < 8; j++) {
            if (dht_wait(port, pin, 1)) return 0;   /* 等50us低结束（高电平开始） */
            delay_us(DHT_BIT_US);                   /* '0'高已结束变低，'1'高仍为高 */
            val <<= 1;
            if (GPIO_ReadInputDataBit(port, pin)) val |= 1;
            if (dht_wait(port, pin, 0)) return 0;   /* 等高结束，进入下一位的50us低 */
        }
        buf[i] = val;
    }

    /* 4) 校验：前4字节之和的低8位 == 第5字节（校验和） */
    if ((u8)(buf[0] + buf[1] + buf[2] + buf[3]) != buf[4]) return 0;
    if (buf[0] == 0 && buf[1] == 0 && buf[2] == 0 && buf[3] == 0) return 0;

    *humi = buf[0];   /* 湿度整数（DHT11 小数位一般为0） */
    *temp = buf[2];   /* 温度整数 */
    return 1;
}
