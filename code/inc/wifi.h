#ifndef __WIFI_H
#define __WIFI_H

#include "stm32f10x.h"

#define WIFI_RX_BUF_LEN  128   /* ESP response buffer */

/* ====== AT state machine ====== */
typedef enum {
    WIFI_OFF = 0,       /* waiting for ESP boot (3s) */
    WIFI_AT,            /* AT test */
    WIFI_ECHO,          /* ATE0 disable echo */
    WIFI_APMODE,        /* AT+CWMODE=2 (AP mode) */
    WIFI_AP,            /* AT+CWSAP (set AP params) */
    WIFI_MUX,           /* AT+CIPMUX=1 */
    WIFI_SERVER,        /* AT+CIPSERVER=1,8080 */
    WIFI_READY,         /* hotspot active, waiting for client */
    WIFI_CONNECTED,     /* TCP client connected */
    WIFI_CIPSEND,       /* sending AT+CIPSEND cmd */
    WIFI_SEND_DATA,     /* sending sensor data frame */
    WIFI_SEND_WAIT      /* waiting for SEND OK */
} wifi_state_t;

extern char WIFI_RX_BUF[WIFI_RX_BUF_LEN];
extern volatile u8 WIFI_RX_STA;

void wifi_init(void);
void wifi_tick(void);
u8   wifi_is_ready(void);
u8   wifi_has_client(void);

#endif /* __WIFI_H */
