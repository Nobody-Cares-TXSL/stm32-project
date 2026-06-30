#ifndef __ALARM_H
#define __ALARM_H

#include "stm32f10x.h"

/* ============================================================
 * 中央联动引擎：传感器采集 + 报警等级判定 + 执行器联动
 * 主循环每 10ms 调一次 alarm_tick()，引擎后台常驻
 * 规则详见 功能联动文档.md
 * ============================================================ */

/* ====== 系统报警等级 ====== */
typedef enum {
    ALARM_NORMAL = 0,   /* 正常 */
    ALARM_L1      = 1,  /* 一级预警 */
    ALARM_L2      = 2,  /* 二级预警 */
    ALARM_L3      = 3   /* 三级确认火警 */
} alarm_level_t;

/* ====== 初始化 / 主循环入口 ====== */
void alarm_init(void);          /* 各传感器 init 之后调用 */
void alarm_tick(void);          /* 主循环每 10ms 调一次 */

/* ====== 引擎开关 ====== */
/* 1=引擎接管执行器(默认,实战)；0=只采集算等级不控制(调试,手动视图正常) */
void alarm_enable(u8 on);
u8   alarm_is_enabled(void);

/* ====== 系统状态 getter（数据聚合中心,供 WiFi/UI 复用） ====== */
u8   alarm_get_level(void);        /* 系统综合等级 0-3 */
u8   alarm_get_temp(void);         /* 温度℃（两路最大） */
u8   alarm_get_humi(void);         /* 湿度% */
u8   alarm_get_temp_ok(void);      /* 温度读数是否有效 1/0 */
u16  alarm_get_smoke(void);        /* 烟雾 ADC 0-4095 */
u16  alarm_get_flame(void);        /* 火焰 ADC（两路最大） 0-4095 */
u16  alarm_get_light(void);        /* 光敏 ADC 0-4095 */
s16  alarm_get_temp_rate(void);    /* 升温速率 x10 ℃/s（15=1.5℃/s） */

/* ====== 单项等级 getter（供 UI 显示单项报警） ====== */
u8   alarm_get_smoke_level(void);  /* 0-3 */
u8   alarm_get_temp_level(void);
u8   alarm_get_flame_level(void);

/* ====== 执行器下发状态 getter（供 WiFi 上报真实下发值） ====== */
u8   alarm_get_fan_pct(void);      /* 引擎下发的风扇% */
u8   alarm_get_pump_on(void);      /* 引擎下发的水泵开关 */

/* ====== 消音接口（本次预留空壳,逻辑后续实现） ====== */
void alarm_mute(void);             /* 本次仅置 mute_flag */
u8   alarm_is_muted(void);         /* 1=消音中 */

/* ====== 执行器手动覆盖（view 层调用） ====== */
#define ALARM_MANUAL_FAN      (1 << 0)
#define ALARM_MANUAL_PUMP     (1 << 1)
#define ALARM_MANUAL_LEDSTRIP (1 << 2)
#define ALARM_MANUAL_BEEP     (1 << 3)

void alarm_manual_override(u8 mask);   /* 设置：进入 view 时调用 */
void alarm_manual_release(u8 mask);   /* 清除：退出 view 时调用 */

/* ====== 光敏等级 ====== */
u8   alarm_get_light_level(void);

/* ====== 逐传感器的原始值 getter（view 显示用） ====== */
u8   alarm_get_temp_n(u8 n);     /* n=1/2, 单路温度 */
u8   alarm_get_humi_n(u8 n);     /* n=1/2, 单路湿度 */
u16  alarm_get_flame_n(u8 n);    /* n=1/2, 单路火焰 AO */
u8   alarm_get_temp_n_ok(u8 n);    /* n=1/2, 按路温度有效 1/0 */
u8   alarm_get_flame_level_n(u8 n);    /* n=1/2, per-sensor flame level */

#endif /* __ALARM_H */
