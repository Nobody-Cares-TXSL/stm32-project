#ifndef __BEEP_H
#define __BEEP_H

#include "stm32f10x.h"

void beep_init(void);
void beep_on(void);
void beep_off(void);
void beep_toggle(void);
u8   beep_get_state(void);  /* 0=OFF, 1=ON */

#endif /* __BEEP_H */
