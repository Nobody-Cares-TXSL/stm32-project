#ifndef __TH_VIEW_H
#define __TH_VIEW_H

#include "stm32f10x.h"

/* 温湿度模块视图三件套（菜单导航的标准形态，后续模块照此复制） */
void th_view_enter(void);              /* 进入：绘制标题/标签/初始数值 */
void th_view_tick(void);               /* 主循环周期调用：节流重读两颗传感器并刷新数值 */
u8   th_view_on_event(u8 ev, u8 sw);   /* 输入处理：DOWN 返回1(退出回主菜单)，其余忽略 */

#endif /* __TH_VIEW_H */
