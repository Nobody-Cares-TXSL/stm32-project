#ifndef __LEDSTRIP_H
#define __LEDSTRIP_H
#include "stm32f10x.h"

/* WS2812B LED strip driver
 * strip 1: PC12(DIN), 7 LEDs
 * strip 2: PC11(DIN), 9 LEDs
 * mode: 0=white, 1=blue blink, 2=yellow blink, 3=red fast blink
 */

#define LEDSTRIP_1      0
#define LEDSTRIP_2      1
#define LS1_MAX         7       /* strip 1 physical LED count */
#define LS2_MAX         9       /* strip 2 physical LED count */
#define LS_MODE_MAX     3       /* mode range 0-3 */

void ledstrip_init(void);                     /* init DIN GPIO, all off */
void ledstrip_set_mode(u8 strip, u8 mode);     /* set mode (0-white .. 3-red) */
u8   ledstrip_get_mode(u8 strip);              /* get current mode */
void ledstrip_tick(void);                      /* call every main loop for blink */
void ledstrip_off(u8 strip);                   /* force off (mode=0, blink=0) */
void ledstrip_all_off(void);                   /* both strips off */

#endif /* __LEDSTRIP_H */
