#include "ledstrip.h"
#include "io.h"        /* GPIO_init */

/* ====== 引脚配置（换线只改这里） ====== */
#define LS1_PORT    GPIOC
#define LS1_PIN     GPIO_Pin_12      /* PC12 = 灯带 1 DIN */
#define LS1_CLK     RCC_APB2Periph_TYPE_C
#define LS2_PORT    GPIOC
#define LS2_PIN     GPIO_Pin_11      /* PC11 = 灯带 2 DIN */
#define LS2_CLK     RCC_APB2Periph_TYPE_C

/* ====== WS2812B 时序校准（72MHz，单位：nop 循环次数） ====== */
#define WS_T0H  2     /* T0H ~0.35us: HIGH 时间 */
#define WS_T0L  6    /* T0L ~0.80us: LOW 时间 */
#define WS_T1H  5    /* T1H ~0.70us: HIGH 时间 */
#define WS_T1L  4     /* T1L ~0.60us: LOW 时间 */

static u8 ls_count[2] = {0, 0};   /* 每条灯带当前点亮数 */

/* nop 循环延迟（约 14ns/nop @72MHz，含循环开销） */
static void ws_delay(u32 n)
{
    while (n--) __nop();
}

/* 发送一个 bit */
static void ws_send_bit(GPIO_TypeDef *port, u16 pin, u8 bit)
{
    if (bit) {
        GPIO_SetBits(port, pin);
        ws_delay(WS_T1H);
        GPIO_ResetBits(port, pin);
        ws_delay(WS_T1L);
    } else {
        GPIO_SetBits(port, pin);
        ws_delay(WS_T0H);
        GPIO_ResetBits(port, pin);
        ws_delay(WS_T0L);
    }
}

/* 发送一个字节（MSB first） */
static void ws_send_byte(GPIO_TypeDef *port, u16 pin, u8 byte)
{
    u8 i;
    for (i = 0; i < 8; i++) {
        ws_send_bit(port, pin, byte & 0x80);
        byte <<= 1;
    }
}

/* 发送 GRB 颜色（WS2812B 字节序：G-R-B） */
static void ws_send_color(GPIO_TypeDef *port, u16 pin, u8 r, u8 g, u8 b)
{
    ws_send_byte(port, pin, g);   /* G first */
    ws_send_byte(port, pin, r);   /* R */
    ws_send_byte(port, pin, b);   /* B */
}

/* 复位（>50us LOW） */
static void ws_reset(GPIO_TypeDef *port, u16 pin)
{
    GPIO_ResetBits(port, pin);
    ws_delay(4000);   /* ~60us */
}

/* 发送一整条灯带的数据 */
static void ledstrip_send(u8 strip, u8 count)
{
    GPIO_TypeDef *port = (strip == LEDSTRIP_1) ? LS1_PORT : LS2_PORT;
    u16 pin             = (strip == LEDSTRIP_1) ? LS1_PIN  : LS2_PIN;
    u8 i;

    __disable_irq();   /* 关中断保证时序 */

    u8 max = (strip == LEDSTRIP_1) ? LS1_MAX : LS2_MAX;
    for (i = 0; i < max; i++) {
        if (i < count)
            ws_send_color(port, pin, 0xFF, 0x00, 0x00);  /* red */
        else
            ws_send_color(port, pin, 0, 0, 0);            /* 灭 */
    }
    ws_reset(port, pin);

    __enable_irq();
}

void ledstrip_init(void)
{
    GPIO_init(LS1_CLK, LS1_PIN, GPIO_Mode_Out_PP, LS1_PORT);
    GPIO_init(LS2_CLK, LS2_PIN, GPIO_Mode_Out_PP, LS2_PORT);
    ledstrip_send(LEDSTRIP_1, 0);   /* 全灭 */
    ledstrip_send(LEDSTRIP_2, 0);
}

void ledstrip_set_count(u8 strip, u8 count)
{
    u8 max2 = (strip == LEDSTRIP_1) ? LS1_MAX : LS2_MAX;
    if (count > max2) count = max2;
    if (strip > LEDSTRIP_2)   return;
    ls_count[strip] = count;
    ledstrip_send(strip, count);
}

u8 ledstrip_get_count(u8 strip)
{
    if (strip > LEDSTRIP_2) return 0;
    return ls_count[strip];
}
