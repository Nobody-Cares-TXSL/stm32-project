#ifndef __FLAME_VIEW_H
#define __FLAME_VIEW_H
#include "stm32f10x.h"

/* 火焰传感器视图（组合视图：同一屏幕显示两个传感器数据）
 * 标准视图三件套：enter / tick / on_event(DOWN 返回)
 */
void flame_view_enter(void);
void flame_view_tick(void);
u8   flame_view_on_event(u8 ev, u8 sw);

#endif /* __FLAME_VIEW_H */
