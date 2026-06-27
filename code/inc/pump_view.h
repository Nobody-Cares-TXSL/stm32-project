#ifndef __PUMP_VIEW_H
#define __PUMP_VIEW_H
#include "stm32f10x.h"

/* 水泵模块视图（菜单子页面：开关控制）
 * 标准视图三件套：enter / tick(空操作) / on_event(SW/DN)
 */
void pump_view_enter(void);
void pump_view_tick(void);
u8   pump_view_on_event(u8 ev, u8 sw);

#endif /* __PUMP_VIEW_H */
