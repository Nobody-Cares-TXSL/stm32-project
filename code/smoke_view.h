#ifndef __SMOKE_VIEW_H
#define __SMOKE_VIEW_H
#include "stm32f10x.h"

/* 烟雾报警器模块视图（菜单进入后显示 AO 浓度 + DO 报警状态）。
 * 标准视图三件套：enter(首次绘制) / tick(周期刷新) / on_event(输入，DOWN 返回)。
 */
void smoke_view_enter(void);
void smoke_view_tick(void);
u8   smoke_view_on_event(u8 ev, u8 sw);

#endif /* __SMOKE_VIEW_H */
