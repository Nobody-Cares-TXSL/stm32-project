#include "ledstrip.h"
#include "io.h"

/* ====== pin config ====== */
#define LS1_PORT    GPIOC
#define LS1_PIN     GPIO_Pin_12      /* PC12 = strip 1 DIN */
#define LS1_CLK     RCC_APB2Periph_TYPE_C
#define LS2_PORT    GPIOC
#define LS2_PIN     GPIO_Pin_11      /* PC11 = strip 2 DIN */
#define LS2_CLK     RCC_APB2Periph_TYPE_C

/*
 * WS2812B timing @72MHz
 * Each __NOP() = 1 cycle = 13.89ns
 * BSRR write = ~1 cycle
 * Inline NOPs (no loop, no function call) => timing exact at any optim level
 *
 * 0-bit: HIGH ~361ns(26cyc), LOW ~847ns(61cyc), total ~1236ns
 * 1-bit: HIGH ~708ns(51cyc), LOW ~458ns(33cyc), total ~1167ns
 * Threshold ~500ns => 0-bit 361ns < 500ns, 1-bit 708ns > 500ns
 */
#define NOP10  __NOP();__NOP();__NOP();__NOP();__NOP(); \
              __NOP();__NOP();__NOP();__NOP();__NOP()

/* ====== blink config ====== */
#define LS_BLINK_TICKS  50   /* ~500ms at 10ms/tick */

/* ====== mode state per strip ====== */
static u8  ls_mode[2]      = {0, 0};
static u8  ls_blink_on[2]  = {0, 0};
static u16 ls_blink_cnt[2] = {0, 0};

/* send one bit - inline NOPs for deterministic timing */
static void ws_send_bit(volatile GPIO_TypeDef *port, u32 high, u32 low, u8 bit)
{
    if (bit) {
        port->BSRR = high;
        NOP10; NOP10; NOP10; NOP10; NOP10;   /* 50 nops */
        port->BSRR = low;
        NOP10; NOP10; NOP10;
        __NOP(); __NOP();                     /* 32 nops total */
    } else {
        port->BSRR = high;
        NOP10; NOP10;
        __NOP(); __NOP(); __NOP(); __NOP(); __NOP();  /* 25 nops total */
        port->BSRR = low;
        NOP10; NOP10; NOP10; NOP10; NOP10;
        NOP10;                                /* 60 nops total */
    }
}

/* send one byte MSB first */
static void ws_send_byte(volatile GPIO_TypeDef *port, u32 high, u32 low, u8 byte)
{
    u8 i;
    for (i = 0; i < 8; i++) {
        ws_send_bit(port, high, low, byte & 0x80);
        byte <<= 1;
    }
}

/* send GRB color (WS2812B byte order: G-R-B) */
static void ws_send_color(volatile GPIO_TypeDef *port, u32 high, u32 low, u8 r, u8 g, u8 b)
{
    ws_send_byte(port, high, low, g);
    ws_send_byte(port, high, low, r);
    ws_send_byte(port, high, low, b);
}

/* reset (>50us LOW) */
static void ws_reset(volatile GPIO_TypeDef *port, u32 low)
{
    u16 i;
    port->BSRR = low;
    for (i = 0; i < 400; i++) __NOP();   /* 400 nops ~5.6us * 9 = ~50us */
}

/* get RGB for mode */
static void mode_color(u8 mode, u8 *r, u8 *g, u8 *b)
{
    switch (mode) {
        case 1: *r=0xFF; *g=0xFF; *b=0x00; break;  /* yellow */
        case 2: *r=0xFF; *g=0x80; *b=0x00; break;  /* orange */
        case 3: *r=0xFF; *g=0x00; *b=0x00; break;  /* red */
        default: *r=0; *g=0; *b=0; break;           /* off */
    }
}

/* send all LEDs for one strip */
static void ledstrip_send(u8 strip)
{
    volatile GPIO_TypeDef *port = (strip == LEDSTRIP_1) ? LS1_PORT : LS2_PORT;
    u16 pin = (strip == LEDSTRIP_1) ? LS1_PIN : LS2_PIN;
    u32 high = pin;
    u32 low  = pin << 16;
    u8 max = (strip == LEDSTRIP_1) ? LS1_MAX : LS2_MAX;
    u8 r, g, b, i;
    u8 on = ls_blink_on[strip] && (ls_mode[strip] != 0);

    __disable_irq();
    for (i = 0; i < max; i++) {
        if (on) {
            mode_color(ls_mode[strip], &r, &g, &b);
            ws_send_color(port, high, low, r, g, b);
        } else {
            ws_send_color(port, high, low, 0, 0, 0);
        }
    }
    ws_reset(port, low);
    __enable_irq();
}

void ledstrip_init(void)
{
    GPIO_init(LS1_CLK, LS1_PIN, GPIO_Mode_Out_PP, LS1_PORT);
    GPIO_init(LS2_CLK, LS2_PIN, GPIO_Mode_Out_PP, LS2_PORT);
    ledstrip_send(LEDSTRIP_1);
    ledstrip_send(LEDSTRIP_2);
}

void ledstrip_set_mode(u8 strip, u8 mode)
{
    if (mode > LS_MODE_MAX) mode = LS_MODE_MAX;
    if (strip > LEDSTRIP_2) return;
    ls_mode[strip] = mode;
    ls_blink_on[strip] = 1;
    ls_blink_cnt[strip] = 0;
    ledstrip_send(strip);
}

u8 ledstrip_get_mode(u8 strip)
{
    if (strip > LEDSTRIP_2) return 0;
    return ls_mode[strip];
}

void ledstrip_tick(void)
{
    u8 s;
    for (s = 0; s < 2; s++) {
        if (ls_mode[s] == 0) continue;
        ls_blink_cnt[s]++;
        if (ls_blink_cnt[s] >= LS_BLINK_TICKS) {
            ls_blink_cnt[s] = 0;
            ls_blink_on[s] = !ls_blink_on[s];
            ledstrip_send(s);
        }
    }
}
