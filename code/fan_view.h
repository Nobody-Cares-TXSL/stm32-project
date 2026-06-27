#ifndef __FAN_VIEW_H
#define __FAN_VIEW_H
#include "stm32f10x.h"

/* 风扇模块视图（菜单子页面：开关控制 + PWM 调速）
 * 标准视图三件套：enter(首次绘制) / tick(空操作) / on_event(SW/L/R/DN)
 */
void fan_view_enter(void);
void fan_view_tick(void);
u8   fan_view_on_event(u8 ev, u8 sw);

#endif /* __FAN_VIEW_H */
