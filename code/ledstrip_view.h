#ifndef __LEDSTRIP_VIEW_H
#define __LEDSTRIP_VIEW_H
#include "stm32f10x.h"

/* 灯带模块视图（菜单子页面：控制两条灯带亮灯数）
 * 标准视图三件套：enter / tick(空操作) / on_event(SW/L/R/DN)
 */
void ledstrip_view_enter(void);
void ledstrip_view_tick(void);
u8   ledstrip_view_on_event(u8 ev, u8 sw);

#endif /* __LEDSTRIP_VIEW_H */
