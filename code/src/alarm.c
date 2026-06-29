#include "alarm.h"
#include "smoke.h"     /* smoke_read_ao */
#include "flame.h"     /* flame_read_ao / FLAME_SENSOR_x */
#include "light.h"     /* light_read_ao */
#include "dht.h"       /* dht_read */
#include "fan.h"       /* fan_set_speed */
#include "pump.h"      /* pump_on/off */
#include "beep.h"      /* beep_on/off/toggle */
#include "ledstrip.h"  /* ledstrip_set_mode / ledstrip_tick */
#include <stdio.h>     /* printf（debug log） */

/* ============================================================
 * 阈值配置（调参只改这里）
 * 约定：value > LN 才算达 N 级（严格大于），与文档">X"一致
 * 例外：火焰 AO 反向（KY-026 有火时拉低），见 classify_flame
 * ============================================================ */
#define ALARM_DEBUG_LOG  /* 调试开关：每秒串口打印 alarm 状态；正式固件注释掉此行 */
#define SMOKE_L1      200U
#define SMOKE_L2      800U
#define SMOKE_L3      2000U
#define SMOKE_SEVERE  2500U      /* 严重超标速判 */

#define TEMP_L1       30
#define TEMP_L2       40
#define TEMP_L3       60
#define TEMP_SEVERE   55         /* 严重超标速判 */

#define RATE_L1       5          /* 升温速率 x10 ℃/s：5=0.5 */
#define RATE_L2       10
#define RATE_L3       25

/* 火焰 AO 反向：有火时 AO 拉低（flame_view 验证 <3000=火），值越小火越大 */
#define FLAME_L1      3000U      /* AO < 此值 = 一级火 */
#define FLAME_L2      2000U      /* AO < 此值 = 二级 */
#define FLAME_L3      1000U      /* AO < 此值 = 三级（阈值待硬件标定） */

/* ============================================================
 * 节流 / 防误报 / 断续计时（单位：10ms tick）
 * ============================================================ */
#define ALARM_TICK_SAMPLE   10      /* 采集+判定节流：100ms */
#define ALARM_TICK_DHT      200     /* DHT 读取节流：2s */
#define ALARM_STABLE_N      3       /* 防误报：连续 3 次(300ms)稳定达标 */

#define BEEP_L1_PERIOD      5000    /* 一级断续周期 ms（5s间隔） */
#define BEEP_L2_PERIOD      1000    /* 二级断续周期 ms（1s间隔） */
/* 三级持续长鸣，无 toggle */

/* DHT 两路引脚（与 th_view 一致） */
#define DHT_PORT1   GPIOB
#define DHT_PIN1    GPIO_Pin_14
#define DHT_PORT2   GPIOB
#define DHT_PIN2    GPIO_Pin_15

/* ====== 采集节流 ====== */
static u16 tick_cnt = 0;

/* ====== 传感器原始值缓存（100ms 刷新） ====== */
static u16 s_smoke   = 0;
static u16 s_flame   = 0;       /* 两路最小（小=火，取最危险） */
static u16 s_light   = 0;
static u8  s_temp    = 0;       /* 两路最大 */
static u8  s_humi    = 0;
static u8  s_temp_ok = 0;
static s16 s_rate    = 0;       /* x10 ℃/s */

/* ====== DHT 节流 + 升温速率 ====== */
static u8  last_temp  = 0;
static u8  dht_inited = 0;      /* 首次读取不算速率（避免 last_temp=0 误判） */

/* ====== 单项等级 ====== */
static u8 lvl_smoke = 0, lvl_temp = 0, lvl_flame = 0, lvl_light = 0;

/* ====== 防误报 ====== */
static u8 target_level = ALARM_NORMAL;
static u8 cur_level    = ALARM_NORMAL;
static u8 stable_cnt   = 0;     /* L1/L2 共用（target 互斥），L3/NORMAL 时清零 */

/* ====== 执行器下发 ====== */
static u8  out_fan_pct     = 0;
static u8  out_pump_on     = 0;
static u8  last_disp_level = 0xFF;   /* 追踪等级变化，避免重复下发 ledstrip */
static u16 beep_period     = 0;      /* 当前 beep 断续周期 ms */
static u16 beep_phase_cnt  = 0;
static u8  beep_cur_on     = 0;

/* ====== 开关 / 消音 ====== */
static u8 enabled   = 1;
static u8 mute_flag = 0;        /* 预留 */
static u16 mute_remain = 0;    /* 静音剩余 tick（10ms 单位） */
static u8  manual_mask = 0;    /* 手动覆盖位掩码 */

static u8  s_t1 = 0, s_h1 = 0, s_t2 = 0, s_h2 = 0;
static u8  s_ok1 = 0, s_ok2 = 0;   /* 按路读取成功状态 */
static u16 s_f1 = 0, s_f2 = 0;

/* 光敏改为二元判定 (s_light<=800=异常)，三级阈值宏已删除 */

/* ============================================================
 * 单项阈值判定
 * smoke/temp/rate：value > LN 达 N 级（值越大越严重）
 * flame 单独反向（见下）
 * ============================================================ */
static u8 classify(u16 v, u16 l1, u16 l2, u16 l3)
{
    if (v > l3) return 3;
    if (v > l2) return 2;
    if (v > l1) return 1;
    return 0;
}

/* 火焰反向：AO 越小越严重（KY-026 有火拉低 AO，flame_view 验证） */
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
    s_flame = (f1 < f2) ? f1 : f2;     s_f1 = f1; s_f2 = f2;     /* 取两路最小（小=火，保守取最危险） */
    s_light = light_read_ao();
}

static void alarm_sample_dht(void)
{
    u8 t1 = 0, h1 = 0, t2 = 0, h2 = 0;
    u8 ok1, ok2;
    u8 new_temp = 0;

    ok1 = dht_read(DHT_PORT1, DHT_PIN1, &t1, &h1);
    ok2 = dht_read(DHT_PORT2, DHT_PIN2, &t2, &h2);

    if (ok1 || ok2) {
        s_temp_ok = 1;
        if (ok1 && t1 > new_temp) new_temp = t1;
        if (ok2 && t2 > new_temp) new_temp = t2;   /* 两路取最大 */
        s_humi = ok1 ? h1 : h2;                    /* 湿度取成功那路 */
        /* 升温速率 x10 ℃/s = (Δtemp / 2s) * 10 = Δtemp * 5 */
        s_rate = dht_inited ? (s16)(((s16)new_temp - (s16)last_temp) * 5) : 0;
        dht_inited = 1;
        last_temp = new_temp;
        s_temp = new_temp;

        if (ok1) { s_t1 = t1; s_h1 = h1; }
        if (ok2) { s_t2 = t2; s_h2 = h2; }
        s_ok1 = ok1; s_ok2 = ok2;

    } else {
        s_temp_ok = 0;     /* 保持上次温度，速率置 0（不误报） */
        s_rate = 0;
    }
}

/* ============================================================
 * 等级判定：单项 → 跨模块规则 + 严重超标速判 → target_level
 * ============================================================ */
static void alarm_eval_levels(void)
{
    u8 lt, lr;

    lvl_smoke = classify(s_smoke, SMOKE_L1, SMOKE_L2, SMOKE_L3);
    lvl_flame = classify_flame(s_flame);
    lvl_light = (s_light <= 800U) ? 1 : 0;   /* 光敏二元: ≤800=异常 */
    if (s_temp_ok) {
        lt = classify((u16)s_temp, (u16)TEMP_L1, (u16)TEMP_L2, (u16)TEMP_L3);
        lr = classify((u16)s_rate, (u16)RATE_L1, (u16)RATE_L2, (u16)RATE_L3);
        lvl_temp = (lt > lr) ? lt : lr;     /* 温度+速率取大 */
    } else {
        lvl_temp = 0;     /* 读数无效不参与 */
    }

    /* 严重超标速判（瞬时三级，无延时） */
    if (s_smoke > SMOKE_SEVERE && s_temp_ok && s_temp > TEMP_SEVERE) {
        target_level = ALARM_L3;
        return;
    }

    /* 三级：烟+温+火焰 三项同时达三级 */
    if (lvl_smoke >= 3 && lvl_temp >= 3 && lvl_flame >= 3) {
        target_level = ALARM_L3;
    }
    /* 二级：烟+温同达二级 或 火焰达二级 */
    else if ((lvl_smoke >= 2 && lvl_temp >= 2) || lvl_flame >= 2) {
        target_level = ALARM_L2;
    }
    /* 一级：烟达一级 且 火焰未达二级 */
    else if (lvl_smoke >= 1 && lvl_flame < 2) {
        target_level = ALARM_L1;
    }
    else {
        target_level = ALARM_NORMAL;
    }
}

/* ============================================================
 * 防误报延时：三级瞬时；一二级连续 N 次稳定才升；target=0 立即回正常
 * （降级 10s 延时为 ★扩展点，本次 target=0 直接回 0）
 * ============================================================ */
static void alarm_apply_stable(void)
{
    /* 三级瞬时触发 */
    if (target_level == ALARM_L3) {
        cur_level = ALARM_L3;
        stable_cnt = 0;
        return;
    }
    /* 一二级：连续 N 次稳定才升（L1/L2 互斥，共用计数器） */
    if (target_level == ALARM_L1 || target_level == ALARM_L2) {
        stable_cnt++;
        if (stable_cnt >= ALARM_STABLE_N && cur_level < target_level)
            cur_level = target_level;
        return;
    }
    /* target=0：本次直接回正常 */
    cur_level = ALARM_NORMAL;
    stable_cnt = 0;
}

/* ============================================================
 * 执行器调度：等级 → 动作矩阵
 * 仅 enabled 时下发；ledstrip 只在等级变化时调（避免重置 blink）
 * ============================================================ */
static void alarm_dispatch_actuators(void)
{
    if (!enabled) return;     /* 调试模式：不接管执行器 */

    switch (cur_level) {
    case ALARM_NORMAL:
        if (!(manual_mask & ALARM_MANUAL_FAN))  { fan_set_speed(0); out_fan_pct = 0; }
        if (!(manual_mask & ALARM_MANUAL_PUMP)) { pump_off();      out_pump_on = 0; }
        beep_period = 0;
        /* ★扩展点：天黑白光常亮（光敏联动） */
        break;
    case ALARM_L1:
        if (!(manual_mask & ALARM_MANUAL_FAN))  { fan_set_speed(30); out_fan_pct = 30; }
        if (!(manual_mask & ALARM_MANUAL_PUMP)) { pump_off();      out_pump_on = 0; }
        beep_period = BEEP_L1_PERIOD;
        break;
    case ALARM_L2:
        if (!(manual_mask & ALARM_MANUAL_FAN))  { fan_set_speed(60); out_fan_pct = 60; }
        /* ★扩展点：pump 二级预加压 */
        if (!(manual_mask & ALARM_MANUAL_PUMP)) { pump_on();       out_pump_on = 1; }
        beep_period = BEEP_L2_PERIOD;
        break;
    case ALARM_L3:
        /* ★扩展点：风扇助燃防护（升温速率达二级且烟雾偏低→不启动） */
        if (!(manual_mask & ALARM_MANUAL_FAN))  { fan_set_speed(100); out_fan_pct = 100; }
        if (!(manual_mask & ALARM_MANUAL_PUMP)) { pump_on();        out_pump_on = 1; }
        /* 三级持续长鸣，drive_beep 处理 */
        /* ★扩展点：ledstrip 三级高频闪（本次用 mode 3 红闪） */
        break;
    }

    /* ledstrip：cur_level 值 0/1/2/3 对应 mode，消音时关闭 */
    if (!(manual_mask & ALARM_MANUAL_LEDSTRIP)) {
        if (mute_flag) {
            if (last_disp_level != 0xFF) {
                ledstrip_all_off();
                last_disp_level = 0xFF;
            }
        } else if (cur_level != last_disp_level) {
            ledstrip_set_mode(LEDSTRIP_1, cur_level);
            ledstrip_set_mode(LEDSTRIP_2, cur_level);
            last_disp_level = cur_level;
        }
    }
}

/* ============================================================
 * beep 断续/长鸣驱动（每 10ms 调）
 * ============================================================ */
static void alarm_drive_beep(void)
{
    if (!enabled) return;     /* 调试模式：不接管 beep */

    /* ★扩展点：消音期间（mute_flag 且非三级）静音 */
    if (mute_flag) {                    /* 消音时蜂鸣器全关（含 L3） */
        if (beep_cur_on) { beep_off(); beep_cur_on = 0; }
        return;
    }

    if (cur_level == ALARM_NORMAL) {
        if (beep_cur_on) { beep_off(); beep_cur_on = 0; }
        beep_phase_cnt = 0;
        return;
    }
    if (cur_level == ALARM_L3) {          /* 持续长鸣 */
        if (!beep_cur_on) { beep_on(); beep_cur_on = 1; }
        return;
    }
    /* L1/L2 断续：按周期 toggle */
    beep_phase_cnt += 10;
    if (beep_phase_cnt >= beep_period / 2) {
        beep_phase_cnt = 0;
        beep_toggle();
        beep_cur_on = !beep_cur_on;
    }
}

/* ============================================================
 * 主循环入口（每 10ms）
 * ============================================================ */
void alarm_tick(void)
{
    tick_cnt++;

    ledstrip_tick();     /* 统一驱动灯带 blink（修复退出视图后不闪的 bug） */

    /* 每 2s：读 DHT + 算升温速率（放在判定前，确保用最新温/速率） */
    if (tick_cnt % ALARM_TICK_DHT == 0) {
        alarm_sample_dht();
    }

    /* 每 100ms：采集 ADC + 判定 + 防误报 + 调度 */
    if (tick_cnt % ALARM_TICK_SAMPLE == 0) {
        alarm_sample_adc();
        alarm_eval_levels();
        alarm_apply_stable();
        alarm_dispatch_actuators();
    }

    /* 每 10ms：beep 断续驱动 */
    alarm_drive_beep();
    if (mute_flag && mute_remain > 0) {
        if (--mute_remain == 0) mute_flag = 0;
    }

#ifdef ALARM_DEBUG_LOG
    /* 每 1s 打印一次状态 */
    {
        static u16 log_cnt = 0;
        if (++log_cnt >= 300) {   /* 300*10ms = 3s */
            log_cnt = 0;
            printf("L%d|S:%d^%d F1:%d^%d F2:%d^%d L:%d^%d T1:%d/%d!%d^%d T2:%d/%d!%d^%d R:%d|F:%d%% P:%d B:L%d D:L%d\r\n",
                   cur_level, s_smoke, lvl_smoke, s_f1, classify_flame(s_f1), s_f2, classify_flame(s_f2), s_light, lvl_light,
                   s_t1, s_h1, s_ok1, lvl_temp, s_t2, s_h2, s_ok2, lvl_temp, s_rate, out_fan_pct, out_pump_on, (!enabled || mute_flag || cur_level == ALARM_NORMAL) ? 0 : cur_level, (mute_flag || last_disp_level == 0xFF) ? 0xFF : last_disp_level);
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
    manual_mask = 0;

    alarm_sample_adc();
    alarm_sample_dht();      /* 首次读，rate=0 */
    alarm_eval_levels();
    alarm_apply_stable();
    alarm_dispatch_actuators();
}

void alarm_enable(u8 on)    { enabled = on; }
u8   alarm_is_enabled(void) { return enabled; }

void alarm_mute(void)    { mute_flag = 1; mute_remain = 1500; }   /* ★扩展点：15s 倒计时后续实现 */
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
/* ====== 光敏等级 ====== */
u8   alarm_get_light_level(void)    { return lvl_light; }

/* ====== 执行器手动覆盖 ====== */
void alarm_manual_override(u8 mask) { manual_mask |= mask; }
void alarm_manual_release(u8 mask)  { manual_mask &= ~mask; }

/* ====== 逐传感器原始值 getter（view 显示用） ====== */
u8   alarm_get_temp_n(u8 n)  { return (n == 1) ? s_t1 : s_t2; }
u8   alarm_get_humi_n(u8 n)  { return (n == 1) ? s_h1 : s_h2; }
u16  alarm_get_flame_n(u8 n) { return (n == 1) ? s_f1 : s_f2; }
u8   alarm_get_temp_n_ok(u8 n) { return (n == 1) ? s_ok1 : s_ok2; }

