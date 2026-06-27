#ifndef __BEEP_VIEW_H
#define __BEEP_VIEW_H

#include "stm32f10x.h"

void beep_view_enter(void);
void beep_view_tick(void);
u8   beep_view_on_event(u8 ev, u8 sw);

#endif /* __BEEP_VIEW_H */
