#ifndef __LIGHT_VIEW_H
#define __LIGHT_VIEW_H
#include "stm32f10x.h"

/* 光敏模块视图（菜单子页面：显示 AO 亮度百分比 + DO 阈值状态）
 * 标准视图三件套：enter(首次绘制) / tick(周期刷新) / on_event(DOWN 返回)
 */
void light_view_enter(void);
void light_view_tick(void);
u8   light_view_on_event(u8 ev, u8 sw);

#endif /* __LIGHT_VIEW_H */
