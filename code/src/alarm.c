#include "alarm.h"
#include "smoke.h"     /* smoke_read_ao */
#include "flame.h"     /* flame_read_ao / FLAME_SENSOR_x */
#include "light.h"     /* light_read_ao */
#include "dht.h"       /* dht_read */
#include "fan.h"       /* fan_set_speed */
#include "pump.h"      /* pump_on/off */
#include "beep.h"      /* beep_on/off/toggle */
#include "ledstrip.h"  /* ledstrip_set_mode / ledstrip_tick */
#include <stdio.h>     /* printf, debug log */

/* ============================================================
 * 阈值配置（集中管理）
 * 约定：value > LN 则为 N 级以上（大于），与文档">X"一致
 * 注意：火焰 AO 反向（KY-026 有火时很低），用 classify_flame
 * ============================================================ */
#define ALARM_DEBUG_LOG  /* 调试开关：每秒串口打印 alarm 状态行，正式部署注释掉即可 */
#define SMOKE_L1      1700U   /* >1700=一级 */
#define SMOKE_L2      2500U   /* >=2500=二级 */
#define SMOKE_L3      3000U   /* >3000=三级 */
#define SMOKE_SEVERE  3500U   /* >3500 + 温度>55 = 瞬时三级 */

#define TEMP_L1       28    /* 28度 */
#define TEMP_L2       30    /* 30度 */
#define TEMP_L3       32    /* 32度 */
#define TEMP_SEVERE   40         /* 严重超温阈值 */

#define RATE_L1       1          /* 0.1 度/秒 */
#define RATE_L2       2           /* 0.2 度/秒 */
#define RATE_L3       3           /* 0.3 度/秒 */

/* 火焰 AO 反向（有火时 AO 很低），flame_view 约定 <3000=火），值越小越危险 */
#define FLAME_L1      3000U      /* AO < 阈值 = 一级火 */
#define FLAME_L2      2000U      /* AO < 阈值 = 二级火 */
#define FLAME_L3      1000U      /* AO < 阈值 = 三级火（阈值需硬件标定） */
#define LIGHT_DARK   3000U      /* 光敏 > 阈值 = 太暗开灯照明 */
#define LIGHT_OFF    2500U      /* 迟滞关灯阈值 */
#define LIGHT_BRIGHT  400U       /* 光敏 < 阈值 = 太亮关灯 */

/* ============================================================
 * 节拍 / 消抖 / 升降级计时（单位：10ms tick）
 * ============================================================ */
#define ALARM_TICK_SAMPLE   10      /* 采集+判定周期：100ms */
#define ALARM_TICK_DHT      200     /* DHT 读取周期：2s */
#define ALARM_STABLE_N      3       /* 防误报：连续 3 次(300ms)稳定才升级 */

#define ALARM_UPGRADE_TICKS 100     /* 升级/降级延时：10s(100*100ms) */
#define BEEP_L1_PERIOD      5000    /* 一级报警蜂鸣 ms：5s 间歇 */
#define BEEP_L2_PERIOD      1000    /* 二级报警蜂鸣 ms：1s 间歇 */
/* 三级蜂鸣持续长鸣，不走 toggle */

/* DHT 双路引脚（与 th_view 一致） */
#define DHT_PORT1   GPIOB
#define DHT_PIN1    GPIO_Pin_14
#define DHT_PORT2   GPIOB
#define DHT_PIN2    GPIO_Pin_15

/* ====== 采集节拍计数器 ====== */
static u16 tick_cnt = 0;

/* ====== 传感器原始值缓存（100ms 刷新） ====== */
static u16 s_smoke   = 0;
static u16 s_flame   = 0;       /* 双路取最小（小=火，保守取危险） */
static u16 s_light   = 0;
static u8  s_temp    = 0;       /* 双路取最大 */
static u8  s_humi    = 0;
static u8  s_temp_ok = 0;
static s16 s_rate    = 0;       /* x10 度/秒 */

/* ====== DHT 缓存 + 温升计算 ====== */
static u8  last_temp  = 0;
static u8  dht_inited = 0;      /* 首次读取跳过温升（避免 last_temp=0 误判） */

/* ====== 单项等级 ====== */
static u8 lvl_smoke = 0, lvl_temp = 0, lvl_flame = 0, lvl_light = 0;

/* ====== 综合等级 ====== */
static u8 target_level = ALARM_NORMAL;
static u8 cur_level    = ALARM_NORMAL;
static u8 stable_cnt   = 0;     /* L1/L2 消抖用（target 未变时累加），L3/NORMAL 时忽略 */
static u16 level_ticks = 0;    /* 当前等级持续 tick，用于升级/降级计时 */
static u8  prev_target = ALARM_NORMAL;  /* 上一次 target，切换时重置计时 */

/* ====== 执行器下发值 ====== */
static u8  out_fan_pct     = 0;
static u8  out_pump_on     = 0;
static u8  last_disp_level = 0xFF;   /* 追踪等级变化，避免重复下发 ledstrip */

static u8  light_led_on   = 0;     /* 1=光控灯带已开白光 */
static u16 beep_period     = 0;      /* 当前 beep 间歇周期 ms */
static u16 beep_phase_cnt  = 0;
static u8  beep_cur_on     = 0;

/* ====== 开关 / 消音 ====== */
static u8 enabled   = 1;
static u8 mute_flag = 0;        /* 消音标志 */
static u16 mute_remain = 0;    /* 消音剩余 tick（10ms 单位） */
static u8  manual_mask = 0;    /* 手动覆盖位掩码 */

static u8  s_t1 = 0, s_h1 = 0, s_t2 = 0, s_h2 = 0;
static u8  s_ok1 = 0, s_ok2 = 0;   /* 双路读取成功状态 */
static u16 s_f1 = 0, s_f2 = 0;

/* 光敏仅二元判断 (s_light<=800=异常)，不参与综合等级判定 */

/* ============================================================
 * 单阈值分级函数
 * smoke/temp/rate：value > LN 为 N 级（值越大越严重）
 * flame 反向（见下）
 * ============================================================ */
static u8 classify(u16 v, u16 l1, u16 l2, u16 l3)
{
    if (v > l3) return 3;
    if (v >= l2) return 2;
    if (v >= l1) return 1;
    return 0;
}

/* 火焰反向 AO 越小越严重（KY-026 有火时 AO 很低，flame_view 约定如此） */
static u8 classify_flame(u16 v)
{
    if (v < FLAME_L3) return 3;
    if (v < FLAME_L2) return 2;
    if (v < FLAME_L1) return 1;
    return 0;
}
/* ============================================================
 * 采集
 * ============================================================ */
static void alarm_sample_adc(void)
{
    u16 f1, f2;
    s_smoke = smoke_read_ao();
    f1 = flame_read_ao(FLAME_SENSOR_1);
    f2 = flame_read_ao(FLAME_SENSOR_2);
    s_flame = (f1 < f2) ? f1 : f2;     s_f1 = f1; s_f2 = f2;     /* 双路取最小（小=火，保守取危险） */
    s_light = light_read_ao();
}

/**
 * @brief 采集 DHT 温湿度传感器数据并更新状态
 * 该函数调用双路 DHT 传感器读取温度和湿度，更新全局温度、湿度值及温升速率。
 * 同时更新双路原始值和有效状态。
 */
static void alarm_sample_dht(void)
{
    u8 t1 = 0, h1 = 0, t2 = 0, h2 = 0;    /* 双路传感器温度湿度缓冲区 */
    u8 ok1, ok2;                          /* 双路传感器读取状态标志 */
    u8 new_temp = 0;

    /* 读取双路传感器温湿度数据 */
    ok1 = dht_read(DHT_PORT1, DHT_PIN1, &t1, &h1);
    ok2 = dht_read(DHT_PORT2, DHT_PIN2, &t2, &h2);

    if (ok1 || ok2) {
        s_temp_ok = 1;
        if (ok1 && t1 > new_temp) new_temp = t1;
        if (ok2 && t2 > new_temp) new_temp = t2;   /* 双路取最大 */
        s_humi = ok1 ? h1 : h2;                    /* 湿度取成功那路 */
        /* 温升速率 x10 度/秒 = (温差 / 2s) * 10 = 温差 * 5 */
        s_rate = dht_inited ? (s16)(((s16)new_temp - (s16)last_temp) * 5) : 0;
        dht_inited = 1;
        last_temp = new_temp;
        s_temp = new_temp;

        if (ok1) { s_t1 = t1; s_h1 = h1; }
        if (ok2) { s_t2 = t2; s_h2 = h2; }
        s_ok1 = ok1; s_ok2 = ok2;

    } else {
        s_temp_ok = 0;     /* 无效时保持上次温度，速率归 0，不误报 */
        s_rate = 0;
        s_ok1 = 0; s_ok2 = 0;
    }
}

/* ============================================================
 * 等级判定引擎：单模块数值 + 跨模块规则 -> target_level
 * ============================================================ */
/**
 * @brief 综合报警等级评估
 * @details 根据烟雾、温度、火焰和光照传感器的读数综合判断当前报警等级
 * @note 该函数修改全局变量 target_level 的值
 */
static void alarm_eval_levels(void)
{
    u8 lt, lr, any_l3, any_l2, any_l1;  /* 局部变量用于存储各类报警等级 */

    /* 分别对各传感器进行等级判定 */
    lvl_smoke = classify(s_smoke, SMOKE_L1, SMOKE_L2, SMOKE_L3);
    /* 对火焰传感器进行等级判定 */
    lvl_flame = classify_flame(s_flame);
    lvl_light = (s_light <= 800U) ? 1 : 0;   /* 光敏二元: <=800=异常 */
    if (s_temp_ok) {
        lt = classify((u16)s_temp, (u16)TEMP_L1, (u16)TEMP_L2, (u16)TEMP_L3);
        lr = (s_rate > 0) ? classify((u16)s_rate, (u16)RATE_L1, (u16)RATE_L2, (u16)RATE_L3) : 0;
        lvl_temp = (lt > lr) ? lt : lr;     /* 温度+速率取高 */
    } else {
        lvl_temp = 0;     /* 传感器无效不参与 */
    }

    /* 严重超限直通：瞬时三级，不延时 */
    if (s_smoke > SMOKE_SEVERE && s_temp_ok && s_temp > TEMP_SEVERE) {
        target_level = ALARM_L3;
        return;
    }

    any_l3 = (lvl_smoke >= 3 || lvl_temp >= 3 || lvl_flame >= 3);
    any_l2 = (lvl_smoke >= 2 || lvl_temp >= 2 || lvl_flame >= 2);
    any_l1 = (lvl_smoke >= 1 || lvl_temp >= 1 || lvl_flame >= 1);

    /* 烟+温+火 三者同时 >= 二级 */
    if (lvl_smoke >= 2 && lvl_temp >= 2 && lvl_flame >= 2) {
        target_level = ALARM_L3;
    }
    /* (烟+温同时 >= 二级) 或 任一单项三级 */
    else if ((lvl_smoke >= 2 && lvl_temp >= 2) || any_l3) {
        target_level = ALARM_L2;
    }
    /* 任一单项一级 且 无二级及以上 */
    else if (any_l1 && !any_l2) {
        target_level = ALARM_L1;
    }
    else {
        target_level = ALARM_NORMAL;
    }
}

/* ============================================================
 * 消抖与延时：瞬时三级，一二需 N 次稳定才升
 * 升级时 L1/L2 每 10s 自动升一档
 * 降级时 target=0 则 10s 后恢复
 * ============================================================ */
static void alarm_apply_stable(void)
{
    /* target 变化则重置升级/降级计时 */
    if (target_level != prev_target) {
        level_ticks = 0;
        prev_target = target_level;
    }
    /* 三级瞬时生效 */
    if (target_level == ALARM_L3) {
        cur_level = ALARM_L3;
        stable_cnt = 0;
        level_ticks = 0;
        return;
    }

    /* 三级逐步降级：L3 未解除，逐级下降 */
    if (cur_level == ALARM_L3 && target_level > ALARM_NORMAL && target_level < ALARM_L3) {
        level_ticks++;
        if (level_ticks >= ALARM_UPGRADE_TICKS) {
            cur_level--;
            level_ticks = 0;
        }
        return;
    }

    /* 二级逐步降级：L2 未解除，逐级下降 */
    if (cur_level == ALARM_L2 && target_level > ALARM_NORMAL && target_level < ALARM_L2) {
        level_ticks++;
        if (level_ticks >= ALARM_UPGRADE_TICKS) {
            cur_level--;
            level_ticks = 0;
        }
        return;
    }

    /* 一二升级：连续 N 次稳定才升 */
    if (target_level == ALARM_L1 || target_level == ALARM_L2) {
        stable_cnt++;
        if (stable_cnt >= ALARM_STABLE_N && cur_level < target_level) {
            cur_level = target_level;
            level_ticks = 0;
        }
        /* 升级时 L1/L2 每 10s 自动升一档（target 未降时） */
        if (cur_level > ALARM_NORMAL && cur_level < ALARM_L3 && target_level > cur_level) {
            level_ticks++;
            if (level_ticks >= ALARM_UPGRADE_TICKS) {
                cur_level++;
                level_ticks = 0;
            }
        }
        return;
    }
    /* target=0 则 10s 稳定后降级 */
    if (cur_level > ALARM_NORMAL) {
        level_ticks++;
        if (level_ticks >= ALARM_UPGRADE_TICKS) {
            cur_level = ALARM_NORMAL;
            level_ticks = 0;
        }
        return;
    }
    cur_level = ALARM_NORMAL;
    stable_cnt = 0;
    level_ticks = 0;
}

/* ============================================================
 * 执行器调度：等级 -> 动作矩阵
 * 仅 enabled 时下发；ledstrip 只在等级变化时下发（避免打断 blink）
 * ============================================================ */
static void alarm_dispatch_actuators(void)
{
    if (!enabled) return;     /* 调试模式，不接管执行器 */

    switch (cur_level) {
        case ALARM_NORMAL:
        if (!(manual_mask & ALARM_MANUAL_FAN))  { fan_set_speed(0); out_fan_pct = 0; }
        beep_period = 0;
        if (!(manual_mask & ALARM_MANUAL_PUMP)) { pump_off(); out_pump_on = 0; }
        /* light-based LED: hysteresis ON>3000 OFF<2500 */
        if (!(manual_mask & ALARM_MANUAL_LEDSTRIP) && !mute_flag) {
            u8 xfer = (last_disp_level >= 1 && last_disp_level <= 3);
            if (s_light > LIGHT_DARK) {
                if (!light_led_on || xfer) {
                    ledstrip_set_mode(LEDSTRIP_1, 0);
                    ledstrip_set_mode(LEDSTRIP_2, 0);
                    light_led_on = 1;
                    last_disp_level = 0;
                }
            } else if (s_light < LIGHT_OFF || xfer) {
                if (light_led_on || xfer) {
                    ledstrip_all_off();
                    light_led_on = 0;
                    last_disp_level = 0xFF;
                }
            }
        }
        break;

    case ALARM_L1:
        if (!(manual_mask & ALARM_MANUAL_FAN))  { fan_set_speed(30); out_fan_pct = 30; }
        beep_period = BEEP_L1_PERIOD;
        break;

    case ALARM_L2:
        if (!(manual_mask & ALARM_MANUAL_FAN))  { fan_set_speed(60); out_fan_pct = 60; }
        beep_period = BEEP_L2_PERIOD;
        break;

    case ALARM_L3:
        /* TODO: 确认三级是否需短时高速排烟而非持续全速 */
        if (!(manual_mask & ALARM_MANUAL_FAN))  { fan_set_speed(100); out_fan_pct = 100; }
        if (!(manual_mask & ALARM_MANUAL_PUMP)) { pump_on();        out_pump_on = 1; }
        /* 蜂鸣器持续长鸣由 drive_beep 处理 */
        /* TODO: ledstrip 三级可增加高频闪（可变 blink 周期），当前 mode 3 红闪 */
        break;
    }

    /* ledstrip: alarm level -> mode */
    if (!(manual_mask & ALARM_MANUAL_LEDSTRIP)) {
        if (mute_flag) {
            if (last_disp_level != 0xFF) {
                ledstrip_all_off();
                light_led_on = 0;
                last_disp_level = 0xFF;
            }
        } else if (cur_level != ALARM_NORMAL && cur_level != last_disp_level) {
            ledstrip_set_mode(LEDSTRIP_1, cur_level);
            ledstrip_set_mode(LEDSTRIP_2, cur_level);
            light_led_on = 0;
            last_disp_level = cur_level;
        }
    }
}

/* ============================================================
 * beep 驱动/消音：每 10ms 调用
 * ============================================================ */
static void alarm_drive_beep(void)
{
    if (!enabled) return;     /* 调试模式，不接管 beep */

    /* TODO: 消音期间（mute_flag 且非三级）仍可响 */
    if (mute_flag) {                    /* 消音时蜂鸣全关（含 L3） */
        if (beep_cur_on) { beep_off(); beep_cur_on = 0; }
        return;
    }

    if (cur_level == ALARM_NORMAL) {
        if (beep_cur_on) { beep_off(); beep_cur_on = 0; }
        beep_phase_cnt = 0;
        return;
    }
    if (cur_level == ALARM_L3) {          /* 三级长鸣 */
        if (!beep_cur_on) { beep_on(); beep_cur_on = 1; }
        return;
    }
    /* L1/L2 间歇鸣叫 toggle */
    beep_phase_cnt += 10;
    if (beep_phase_cnt >= beep_period / 2) {
        beep_phase_cnt = 0;
        beep_toggle();
        beep_cur_on = !beep_cur_on;
    }
}

/* ============================================================
 * 主循环入口，每 10ms 调用
 * ============================================================ */
void alarm_tick(void)
{
    tick_cnt++;

    if (enabled) {
        ledstrip_tick();  /* 统一驱动灯带 blink */
    }

    /* 每 2s：采集 DHT + 计算温升（放判定前确保温升数据就绪） */
    if (tick_cnt % ALARM_TICK_DHT == 0) {
        alarm_sample_dht();
    }

    /* 每 100ms：采集 ADC + 判定 + 消抖 + 调度 */
    if (tick_cnt % ALARM_TICK_SAMPLE == 0) {
        alarm_sample_adc();
        alarm_eval_levels();
        alarm_apply_stable();
        alarm_dispatch_actuators();
    }

    /* 每 10ms：beep 驱动 */
    alarm_drive_beep();
    if (mute_flag && mute_remain > 0) {
        if (--mute_remain == 0) mute_flag = 0;
    }

#ifdef ALARM_DEBUG_LOG
    /* 每 3s 打印一次状态 */
    {
        static u16 log_cnt = 0;
        if (++log_cnt >= 300) {   /* 300*10ms = 3s */
            log_cnt = 0;
            printf("L%d|S:%d^%d F1:%d^%d F2:%d^%d L:%d^%d T1:%d/%d!%d^%d T2:%d/%d!%d^%d R:%d|F:%d%% P:%d B:L%d D:L%d\r\n",
                   cur_level, s_smoke, lvl_smoke, s_f1, classify_flame(s_f1), s_f2, classify_flame(s_f2), s_light, lvl_light,
                   s_t1, s_h1, s_ok1, lvl_temp, s_t2, s_h2, s_ok2, lvl_temp, s_rate, (cur_level == ALARM_NORMAL) ? 0 : (cur_level == ALARM_L1) ? 30 : (cur_level == ALARM_L2) ? 60 : 100, (cur_level >= ALARM_L2) ? 1 : 0, (mute_flag || cur_level == ALARM_NORMAL) ? 0 : cur_level, (mute_flag) ? 0xFF : cur_level);
        }
    }
#endif
}

/* ============================================================
 * 初始化 / 开关 / 消音
 * ============================================================ */
void alarm_init(void)
{
    enabled   = 1;
    mute_flag = 0;
    cur_level    = ALARM_NORMAL;
    target_level = ALARM_NORMAL;
    last_disp_level = 0xFF;
    dht_inited = 0;
    stable_cnt = 0;
    level_ticks = 0;
    prev_target = ALARM_NORMAL;
    manual_mask = 0;

    alarm_sample_adc();
    alarm_sample_dht();      /* 首次读取 rate=0 */
    alarm_eval_levels();
    alarm_apply_stable();
    alarm_dispatch_actuators();
}

void alarm_enable(u8 on)    { enabled = on; if (on) { light_led_on = 0; last_disp_level = 0xFF; } }
u8   alarm_is_enabled(void) { return enabled; }

void alarm_mute(void)    { mute_flag = 1; mute_remain = 1500; }   /* TODO: 15s 消音定时（待实现） */
u8   alarm_is_muted(void){ return mute_flag; }

/* ============================================================
 * getter（数据聚合中心）
 * ============================================================ */
u8  alarm_get_level(void)     { return cur_level; }
u8  alarm_get_temp(void)      { return s_temp; }
u8  alarm_get_humi(void)      { return s_humi; }
u8  alarm_get_temp_ok(void)   { return s_temp_ok; }
u16 alarm_get_smoke(void)     { return s_smoke; }
u16 alarm_get_flame(void)     { return s_flame; }
u16 alarm_get_light(void)     { return s_light; }
s16 alarm_get_temp_rate(void) { return s_rate; }
u8  alarm_get_smoke_level(void) { return lvl_smoke; }
u8  alarm_get_temp_level(void)  { return lvl_temp; }
u8  alarm_get_flame_level(void) { return lvl_flame; }
u8  alarm_get_fan_pct(void)     { return out_fan_pct; }
u8  alarm_get_pump_on(void)     { return out_pump_on; }
/* ====== 光照等级 ====== */
u8   alarm_get_light_level(void)    { return lvl_light; }

/* ====== 执行器手动覆盖 ====== */
void alarm_manual_override(u8 mask) { manual_mask |= mask; }
void alarm_manual_release(u8 mask)  { manual_mask &= ~mask; }

/* ====== 双路传感器原始值 getter（view 显示用） ====== */
u8   alarm_get_temp_n(u8 n)  { return (n == 1) ? s_t1 : s_t2; }
u8   alarm_get_humi_n(u8 n)  { return (n == 1) ? s_h1 : s_h2; }
u16  alarm_get_flame_n(u8 n) { return (n == 1) ? s_f1 : s_f2; }
u8   alarm_get_temp_n_ok(u8 n) { return (n == 1) ? s_ok1 : s_ok2; }

u8   alarm_get_flame_level_n(u8 n) { return classify_flame((n == 1) ? s_f1 : s_f2); }
