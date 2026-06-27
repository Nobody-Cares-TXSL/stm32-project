#include "ledstrip.h"
#include "io.h"

/* ====== pin config ====== */
#define LS1_PORT    GPIOC
#define LS1_PIN     GPIO_Pin_12      /* PC12 = strip 1 DIN */
#define LS1_CLK     RCC_APB2Periph_TYPE_C
#define LS2_PORT    GPIOC
#define LS2_PIN     GPIO_Pin_11      /* PC11 = strip 2 DIN */
#define LS2_CLK     RCC_APB2Periph_TYPE_C

/* ====== WS2812B timing @72MHz (direct BSRR, nop loop ~42ns/iter) ====== */
#define WS_T0H  7     /* T0H ~0.35us: HIGH time */
#define WS_T0L  22    /* T0L ~0.80us: LOW time */
#define WS_T1H  16    /* T1H ~0.70us: HIGH time */
#define WS_T1L  8     /* T1L ~0.60us: LOW time */

/* ====== blink config ====== */
#define LS_BLINK_TICKS  50   /* ~500ms at 10ms/tick */

/* ====== mode state per strip ====== */
static u8  ls_mode[2]      = {0, 0};
static u8  ls_blink_on[2]  = {0, 0};
static u16 ls_blink_cnt[2] = {0, 0};

/* nop loop delay */
static void ws_delay(u32 n)
{
    while (n--) __nop();
}

/* send one bit (direct BSRR register access) */
static void ws_send_bit(GPIO_TypeDef *port, u16 pin, u8 bit)
{
    if (bit) {
        port->BSRR = pin;         /* set HIGH */
        ws_delay(WS_T1H);
        port->BSRR = pin << 16;   /* set LOW */
        ws_delay(WS_T1L);
    } else {
        port->BSRR = pin;         /* set HIGH */
        ws_delay(WS_T0H);
        port->BSRR = pin << 16;   /* set LOW */
        ws_delay(WS_T0L);
    }
}

/* send one byte MSB first */
static void ws_send_byte(GPIO_TypeDef *port, u16 pin, u8 byte)
{
    u8 i;
    for (i = 0; i < 8; i++) {
        ws_send_bit(port, pin, byte & 0x80);
        byte <<= 1;
    }
}

/* send GRB color (WS2812B byte order: G-R-B) */
static void ws_send_color(GPIO_TypeDef *port, u16 pin, u8 r, u8 g, u8 b)
{
    ws_send_byte(port, pin, g);
    ws_send_byte(port, pin, r);
    ws_send_byte(port, pin, b);
}

/* reset (>50us LOW) */
static void ws_reset(GPIO_TypeDef *port, u16 pin)
{
    port->BSRR = pin << 16;
    ws_delay(4000);   /* ~60us */
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
    GPIO_TypeDef *port = (strip == LEDSTRIP_1) ? LS1_PORT : LS2_PORT;
    u16 pin = (strip == LEDSTRIP_1) ? LS1_PIN : LS2_PIN;
    u8 max = (strip == LEDSTRIP_1) ? LS1_MAX : LS2_MAX;
    u8 r, g, b, i;
    u8 on = ls_blink_on[strip] && (ls_mode[strip] != 0);

    __disable_irq();
    for (i = 0; i < max; i++) {
        if (on) {
            mode_color(ls_mode[strip], &r, &g, &b);
            ws_send_color(port, pin, r, g, b);
        } else {
            ws_send_color(port, pin, 0, 0, 0);
        }
    }
    ws_reset(port, pin);
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
        if (ls_mode[s] == 0) continue;   /* off, skip */
        ls_blink_cnt[s]++;
        if (ls_blink_cnt[s] >= LS_BLINK_TICKS) {
            ls_blink_cnt[s] = 0;
            ls_blink_on[s] = !ls_blink_on[s];
            ledstrip_send(s);
        }
    }
}
