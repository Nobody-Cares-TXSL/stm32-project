#include "wifi.h"
#include "io.h"
#include "usart.h"
#include "alarm.h"
#include <stdio.h>
#include <string.h>

/* ====== USART2 pin / baud ====== */
#define WIFI_PORT      GPIOA
#define WIFI_TX_PIN    GPIO_Pin_2
#define WIFI_RX_PIN    GPIO_Pin_3
#define WIFI_CLK_TYPE  RCC_APB2Periph_TYPE_A
#define WIFI_BAUD      115200

/* ====== RX buffer (ISR writes, main loop reads) ====== */
char WIFI_RX_BUF[WIFI_RX_BUF_LEN];
volatile u8 WIFI_RX_STA = 0;
static u16 wifi_rx_idx = 0;

/* ====== AT state machine vars ====== */
static wifi_state_t wifi_state = WIFI_OFF;
static u8  at_phase  = 0;       /* 0=SEND, 1=WAIT */
static u16 at_tick   = 0;       /* timeout counter */
static u8  at_retry  = 0;

/* ====== startup delay (WIFI_OFF state) ====== */
static u16 boot_ticks = 0;

/* ====== data send interval ====== */
static u16 send_tick = 0;

/* ====== pre-built sensor frame ====== */
static char frame_buf[96];
static u8   frame_len = 0;

/* ====== AT init commands, one per state WIFI_AT..WIFI_SERVER ====== */
static const char *at_cmds[] = {
    "AT\r\n",
    "ATE0\r\n",
    "AT+CWMODE=2\r\n",
    "AT+CWSAP=\"FireAlarm\",\"\",1,0\r\n",
    "AT+CIPMUX=1\r\n",
    "AT+CIPSERVER=1,8080\r\n",
};
#define AT_CMD_COUNT  (sizeof(at_cmds) / sizeof(at_cmds[0]))

#define AT_TIMEOUT     200   /* 2s @ 10ms/tick */
#define AT_MAX_RETRY   3
#define BOOT_DELAY     300   /* 3s ESP boot wait */
#define SEND_INTERVAL  100   /* 1s between data frames */

/* ====== helpers ====== */
static void wifi_rx_clear(void) { WIFI_RX_STA = 0; }

/* check RX buffer for substring (consumes STA if match) */
static u8 wifi_rx_has(const char *s)
{
    if (!WIFI_RX_STA) return 0;
    if (strstr(WIFI_RX_BUF, s) != NULL) {
        wifi_rx_clear();
        return 1;
    }
    return 0;
}

/* ====== USART2 init ====== */
static void wifi_usart2_init(void)
{
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
    GPIO_init(WIFI_CLK_TYPE, WIFI_TX_PIN, GPIO_Mode_AF_PP, WIFI_PORT);
    GPIO_init(WIFI_CLK_TYPE, WIFI_RX_PIN, GPIO_Mode_IPU, WIFI_PORT);
    USART_init(USART2, WIFI_BAUD, USART_WordLength_8b, USART_StopBits_1,
               USART_Parity_No, USART_Mode_Rx | USART_Mode_Tx);
    USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
    USART_Cmd(USART2, ENABLE);
    NVIC_init(USART2_IRQn, 1, 1);
}

/* ====== USART2 send ====== */
static void wifi_send_str(const char *s)
{
    while (*s) {
        while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
        USART_SendData(USART2, *s++);
    }
}

/* ====== build sensor data frame, return length ====== */
static u8 build_frame(void)
{
    u8 cur = alarm_get_level();
    u8 fan_pct, pump_on, beep_lvl, led_lvl;

    switch (cur) {
        case ALARM_NORMAL: fan_pct = 0;   break;
        case ALARM_L1:     fan_pct = 30;  break;
        case ALARM_L2:     fan_pct = 60;  break;
        default:           fan_pct = 100; break;
    }
    pump_on = (cur >= ALARM_L2) ? 1 : 0;
    beep_lvl = (alarm_is_muted() || cur == ALARM_NORMAL) ? 0 : cur;
    led_lvl  = alarm_is_muted() ? 0xFF : cur;

    return (u8)sprintf(frame_buf,
        "L%d|S:%d^%d F1:%d^%d F2:%d^%d L:%d^%d T1:%d/%d!%d^%d T2:%d/%d!%d^%d R:%d|F:%d%% P:%d B:L%d D:L%d\r\n",
        cur,
        alarm_get_smoke(), alarm_get_smoke_level(),
        alarm_get_flame_n(1), alarm_get_flame_level_n(1),
        alarm_get_flame_n(2), alarm_get_flame_level_n(2),
        alarm_get_light(), alarm_get_light_level(),
        alarm_get_temp_n(1), alarm_get_humi_n(1), alarm_get_temp_n_ok(1), alarm_get_temp_level(),
        alarm_get_temp_n(2), alarm_get_humi_n(2), alarm_get_temp_n_ok(2), alarm_get_temp_level(),
        alarm_get_temp_rate(),
        fan_pct, pump_on, beep_lvl, led_lvl);
}

/* ====== run one AT init step, returns: 0=waiting 1=OK 2=timeout 3=ERROR ====== */
static u8 at_init_step(u8 cmd_idx)
{
    if (at_phase == 0) {  /* SEND */
        printf("[WIFI] TX: %s", at_cmds[cmd_idx]);
        wifi_send_str(at_cmds[cmd_idx]);
        at_phase = 1;
        at_tick  = 0;
        return 0;
    }

    /* WAIT for response */
    at_tick++;
    if (at_tick >= AT_TIMEOUT) {
        printf("[WIFI] TIMEOUT\r\n");
        at_phase = 0;
        return 2;
    }
    if (!WIFI_RX_STA) return 0;

    /* got a line */
    printf("[WIFI] RX: %s", WIFI_RX_BUF);
    if (strstr(WIFI_RX_BUF, "OK") || strstr(WIFI_RX_BUF, "ALREADY")) {
        wifi_rx_clear();
        at_phase = 0;
        at_retry = 0;
        return 1;
    }
    if (strstr(WIFI_RX_BUF, "ERROR") || strstr(WIFI_RX_BUF, "FAIL")) {
        wifi_rx_clear();
        at_phase = 0;
        return 3;
    }
    /* unsolicited line (e.g. busy, link status), ignore and keep waiting */
    wifi_rx_clear();
    return 0;
}

/* ====== public API ====== */

void wifi_init(void)
{
    wifi_usart2_init();
    wifi_state = WIFI_OFF;
    at_phase   = 0;
    at_retry   = 0;
    at_tick    = 0;
    boot_ticks = 0;
    send_tick  = 0;
    printf("[WIFI] init OK, waiting for ESP boot...\r\n");
}

u8 wifi_is_ready(void)   { return (wifi_state >= WIFI_READY); }
u8 wifi_has_client(void) { return (wifi_state >= WIFI_CONNECTED); }

/* ====== main tick (call every 10ms from superloop) ====== */
void wifi_tick(void)
{
    u8 result, cmd_idx;

    switch (wifi_state) {

    /* ---- wait for ESP-01S to boot ---- */
    case WIFI_OFF:
        boot_ticks++;
        if (boot_ticks >= BOOT_DELAY) {
            wifi_state = WIFI_AT;
            at_phase   = 0;
            at_retry   = 0;
            printf("[WIFI] START AT INIT\r\n");
        }
        break;

    /* ---- AT init sequence (states WIFI_AT..WIFI_SERVER) ---- */
    case WIFI_AT:
    case WIFI_ECHO:
    case WIFI_APMODE:
    case WIFI_AP:
    case WIFI_MUX:
    case WIFI_SERVER:
        cmd_idx = (u8)wifi_state - (u8)WIFI_AT;
        result  = at_init_step(cmd_idx);
        if (result == 1) {
            /* OK, advance to next init state */
            wifi_state = (wifi_state_t)((u8)wifi_state + 1);
            printf("[WIFI] STEP %d OK\r\n", cmd_idx + 1);
        } else if (result == 2 || result == 3) {
            /* timeout or ERROR */
            at_retry++;
            if (at_retry >= AT_MAX_RETRY) {
                printf("[WIFI] STEP %d FAIL x%d, restart\r\n", cmd_idx + 1, AT_MAX_RETRY);
                wifi_state = WIFI_OFF;
                boot_ticks = 0;
                at_retry   = 0;
            }
            /* else retry same step (at_phase already reset in at_init_step) */
        }
        break;

    /* ---- hotspot active, waiting for TCP client ---- */
    case WIFI_READY:
        if (wifi_rx_has("CONNECT")) {
            printf("[WIFI] CLIENT CONNECTED\r\n");
            wifi_state = WIFI_CONNECTED;
            send_tick  = SEND_INTERVAL;   /* send first frame ASAP */
        }
        break;

    /* ---- TCP client connected, send data periodically ---- */
    case WIFI_CONNECTED:
        send_tick++;
        if (wifi_rx_has("CLOSED")) {
            printf("[WIFI] CLIENT DISCONNECTED\r\n");
            wifi_state = WIFI_READY;
            break;
        }
        if (send_tick >= SEND_INTERVAL) {
            send_tick = 0;
            frame_len = build_frame();
            wifi_state = WIFI_CIPSEND;
            at_phase   = 0;
            at_tick    = 0;
            at_retry   = 0;
        }
        break;

    /* ---- CIPSEND: send AT+CIPSEND=0,<len> ---- */
    case WIFI_CIPSEND:
        if (at_phase == 0) {
            char cmd[24];
            sprintf(cmd, "AT+CIPSEND=0,%d\r\n", frame_len);
            printf("[WIFI] TX: %s", cmd);
            wifi_send_str(cmd);
            at_phase = 1;
            at_tick  = 0;
        } else {
            at_tick++;
            if (at_tick >= 100) {   /* 1s timeout */
                printf("[WIFI] CIPSEND timeout\r\n");
                at_phase = 0;
                wifi_state = WIFI_CONNECTED;
                break;
            }
            if (!WIFI_RX_STA) break;
            printf("[WIFI] RX: %s", WIFI_RX_BUF);
            if (strchr(WIFI_RX_BUF, '>') != NULL) {
                wifi_rx_clear();
                wifi_state = WIFI_SEND_DATA;
                at_phase   = 0;
            } else if (strstr(WIFI_RX_BUF, "ERROR")) {
                wifi_rx_clear();
                at_phase = 0;
                wifi_state = WIFI_CONNECTED;  /* skip this frame, retry next */
            } else {
                wifi_rx_clear();  /* ignore other lines */
            }
        }
        break;

    /* ---- SEND_DATA: push sensor frame ---- */
    case WIFI_SEND_DATA:
        wifi_send_str(frame_buf);
        at_tick    = 0;
        wifi_state = WIFI_SEND_WAIT;
        break;

    /* ---- SEND_WAIT: wait for SEND OK ---- */
    case WIFI_SEND_WAIT:
        at_tick++;
        if (at_tick >= 100) {   /* 1s timeout */
            printf("[WIFI] SEND OK timeout\r\n");
            wifi_state = WIFI_CONNECTED;
            break;
        }
        if (!WIFI_RX_STA) break;
        printf("[WIFI] RX: %s", WIFI_RX_BUF);
        if (strstr(WIFI_RX_BUF, "SEND OK")) {
            wifi_rx_clear();
            wifi_state = WIFI_CONNECTED;
        } else if (strstr(WIFI_RX_BUF, "SEND FAIL") || strstr(WIFI_RX_BUF, "ERROR")) {
            wifi_rx_clear();
            wifi_state = WIFI_CONNECTED;
        } else {
            wifi_rx_clear();  /* ignore */
        }
        break;

    default:
        break;
    }

    /* drain unsolicited RX lines */
    if (WIFI_RX_STA) {
        wifi_rx_clear();
    }
}

/* ====== USART2 ISR: forward to USART1 log + buffer with null-term ====== */
void USART2_IRQHandler(void)
{
    if (USART_GetITStatus(USART2, USART_IT_RXNE) != RESET) {
        char ch = USART_ReceiveData(USART2);
        USART1_send_byte(ch);       /* echo to debug log */
        if (wifi_rx_idx < WIFI_RX_BUF_LEN - 1) {
            WIFI_RX_BUF[wifi_rx_idx++] = ch;
            if (ch == '\n') {
                WIFI_RX_BUF[wifi_rx_idx] = '\0';   /* null-terminate for strstr */
                WIFI_RX_STA = 1;
                wifi_rx_idx = 0;
            }
        }
        USART_ClearITPendingBit(USART2, USART_IT_RXNE);
    }
}
