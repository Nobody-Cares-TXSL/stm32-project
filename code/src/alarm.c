#include "alarm.h"
#include "smoke.h"     /* smoke_read_ao */
#include "flame.h"     /* flame_read_ao / FLAME_SENSOR_x */
#include "light.h"     /* light_read_ao */
#include "dht.h"       /* dht_read */
#include "fan.h"       /* fan_set_speed */
#include "pump.h"      /* pump_on/off */
#include "beep.h"      /* beep_on/off/toggle */
#include "ledstrip.h"  /* ledstrip_set_mode / ledstrip_tick */
#include <stdio.h>     /* printf��debug log�� */

/* ============================================================
 * ��ֵ���ã�����ֻ�����
 * Լ����value > LN ����� N �����ϸ���ڣ������ĵ�">X"һ��
 * ���⣺���� AO ����KY-026 �л�ʱ���ͣ����� classify_flame
 * ============================================================ */
#define ALARM_DEBUG_LOG  /* ���Կ��أ�ÿ�봮�ڴ�ӡ alarm ״̬����ʽ�̼�ע�͵����� */
#define SMOKE_L1      1700U   /* ��1700=һ�� */
#define SMOKE_L2      2500U   /* ��2500=���� */
#define SMOKE_L3      3000U   /* >3000=���� */
#define SMOKE_SEVERE  3500U   /* >3500 + �¶�>55 �� ˲ʱ���� */

#define TEMP_L1       32    /* 32�� */
#define TEMP_L2       35    /* 35�� */
#define TEMP_L3       37    /* 37�� */
#define TEMP_SEVERE   55         /* ���س������� */

#define RATE_L1       1          /* 0.1 ��/s */
#define RATE_L2       2           /* 0.2 ��/s */
#define RATE_L3       3           /* 0.3 ��/s */

/* ���� AO �����л�ʱ AO ���ͣ�flame_view ��֤ <3000=�𣩣�ֵԽС��Խ�� */
#define FLAME_L1      3000U      /* AO < ��ֵ = һ���� */
#define FLAME_L2      2000U      /* AO < ��ֵ = ���� */
#define FLAME_L3      1000U      /* AO < ��ֵ = ��������ֵ��Ӳ���궨�� */
#define LIGHT_DARK   3000U      /* ���� > ��ֵ = ̫�������ƴ� */
#define LIGHT_OFF    2500U      /* hyst off threshold */
#define LIGHT_BRIGHT  400U       /* ���� < ��ֵ = �������صƴ� */

/* ============================================================
 * ���� / ���� / ������ʱ����λ��10ms tick��
 * ============================================================ */
#define ALARM_TICK_SAMPLE   10      /* �ɼ�+�ж�������100ms */
#define ALARM_TICK_DHT      200     /* DHT ��ȡ������2s */
#define ALARM_STABLE_N      3       /* ���󱨣����� 3 ��(300ms)�ȶ���� */

#define ALARM_UPGRADE_TICKS 100     /* ����/������ʱ��10s(100*100ms) */
#define BEEP_L1_PERIOD      5000    /* һ���������� ms��5s����� */
#define BEEP_L2_PERIOD      1000    /* ������������ ms��1s����� */
/* ���������������� toggle */

/* DHT ��·���ţ��� th_view һ�£� */
#define DHT_PORT1   GPIOB
#define DHT_PIN1    GPIO_Pin_14
#define DHT_PORT2   GPIOB
#define DHT_PIN2    GPIO_Pin_15

/* ====== �ɼ����� ====== */
static u16 tick_cnt = 0;

/* ====== ������ԭʼֵ���棨100ms ˢ�£� ====== */
static u16 s_smoke   = 0;
static u16 s_flame   = 0;       /* ��·��С��С=��ȡ��Σ�գ� */
static u16 s_light   = 0;
static u8  s_temp    = 0;       /* ��·��� */
static u8  s_humi    = 0;
static u8  s_temp_ok = 0;
static s16 s_rate    = 0;       /* x10 ��/s */

/* ====== DHT ���� + �������� ====== */
static u8  last_temp  = 0;
static u8  dht_inited = 0;      /* �״ζ�ȡ�������ʣ����� last_temp=0 ���У� */

/* ====== ����ȼ� ====== */
static u8 lvl_smoke = 0, lvl_temp = 0, lvl_flame = 0, lvl_light = 0;

/* ====== ���� ====== */
static u8 target_level = ALARM_NORMAL;
static u8 cur_level    = ALARM_NORMAL;
static u8 stable_cnt   = 0;     /* L1/L2 ���ã�target ���⣩��L3/NORMAL ʱ���� */
static u16 level_ticks = 0;    /* ��ǰ�ȼ����� tick ������������/���� */
static u8  prev_target = ALARM_NORMAL;  /* ��һ�� target���л�ʱ���ü�ʱ */

/* ====== ִ�����·� ====== */
static u8  out_fan_pct     = 0;
static u8  out_pump_on     = 0;
static u8  last_disp_level = 0xFF;   /* ׷�ٵȼ��仯�������ظ��·� ledstrip */

static u8  light_led_on   = 0;     /* 1=block1 turned white on */
static u16 beep_period     = 0;      /* ��ǰ beep �������� ms */
static u16 beep_phase_cnt  = 0;
static u8  beep_cur_on     = 0;

/* ====== ���� / ���� ====== */
static u8 enabled   = 1;
static u8 mute_flag = 0;        /* Ԥ�� */
static u16 mute_remain = 0;    /* ����ʣ�� tick��10ms ��λ�� */
static u8  manual_mask = 0;    /* �ֶ�����λ���� */

static u8  s_t1 = 0, s_h1 = 0, s_t2 = 0, s_h2 = 0;
static u8  s_ok1 = 0, s_ok2 = 0;   /* ��·��ȡ�ɹ�״̬ */
static u16 s_f1 = 0, s_f2 = 0;

/* ������Ϊ��Ԫ�ж� (s_light<=800=�쳣)��������ֵ����ɾ�� */

/* ============================================================
 * ������ֵ�ж�
 * smoke/temp/rate��value > LN �� N ����ֵԽ��Խ���أ�
 * flame �������򣨼��£�
 * ============================================================ */
static u8 classify(u16 v, u16 l1, u16 l2, u16 l3)
{
    if (v > l3) return 3;
    if (v >= l2) return 2;
    if (v >= l1) return 1;
    return 0;
}

/* ���淴��AO ԽСԽ���أ�KY-026 �л����� AO��flame_view ��֤�� */
static u8 classify_flame(u16 v)
{
    if (v < FLAME_L3) return 3;
    if (v < FLAME_L2) return 2;
    if (v < FLAME_L1) return 1;
    return 0;
}
/* ============================================================
 * �ɼ�
 * ============================================================ */
static void alarm_sample_adc(void)
{
    u16 f1, f2;
    s_smoke = smoke_read_ao();
    f1 = flame_read_ao(FLAME_SENSOR_1);
    f2 = flame_read_ao(FLAME_SENSOR_2);
    s_flame = (f1 < f2) ? f1 : f2;     s_f1 = f1; s_f2 = f2;     /* ȡ��·��С��С=�𣬱���ȡ��Σ�գ� */
    s_light = light_read_ao();
}

/**
 * @brief �ɼ�DHT��ʪ�ȴ��������ݲ�����״̬
 * �ú���������DHT��������ȡ�¶Ⱥ�ʪ�ȣ���������¶ȡ�ʪ��ֵ���¶ȱ仯��
 * ������ȫ��״̬����
 */
static void alarm_sample_dht(void)
{
    u8 t1 = 0, h1 = 0, t2 = 0, h2 = 0;    // �������������¶Ⱥ�ʪ������
    u8 ok1, ok2;                          // ������������ȡ״̬��־
    u8 new_temp = 0;

    // ��ȡ��������������ʪ������
    ok1 = dht_read(DHT_PORT1, DHT_PIN1, &t1, &h1);
    ok2 = dht_read(DHT_PORT2, DHT_PIN2, &t2, &h2);

    if (ok1 || ok2) {
        s_temp_ok = 1;
        if (ok1 && t1 > new_temp) new_temp = t1;
        if (ok2 && t2 > new_temp) new_temp = t2;   /* ��·ȡ��� */
        s_humi = ok1 ? h1 : h2;                    /* ʪ��ȡ�ɹ���· */
        /* �������� x10 ��/s = (��temp / 2s) * 10 = ��temp * 5 */
        s_rate = dht_inited ? (s16)(((s16)new_temp - (s16)last_temp) * 5) : 0;
        dht_inited = 1;
        last_temp = new_temp;
        s_temp = new_temp;

        if (ok1) { s_t1 = t1; s_h1 = h1; }
        if (ok2) { s_t2 = t2; s_h2 = h2; }
        s_ok1 = ok1; s_ok2 = ok2;

    } else {
        s_temp_ok = 0;     /* �����ϴ��¶ȣ������� 0�����󱨣� */
        s_rate = 0;
        s_ok1 = 0; s_ok2 = 0;
    }
}

/* ============================================================
 * �ȼ��ж������� �� ��ģ����� + ���س������� �� target_level
 * ============================================================ */
/**
 * @brief ���������ȼ�
 * @details �����������¶ȡ�����͹��մ������Ķ������ۺ��жϵ�ǰ�����ȼ�
 * @note �ú��������ȫ�ֱ���target_level��ֵ
 */
static void alarm_eval_levels(void)
{
    u8 lt, lr, any_l3, any_l2, any_l1;  // ����ֲ����������ڴ洢���౨���ȼ�

    /* ���������������еȼ����� */
    lvl_smoke = classify(s_smoke, SMOKE_L1, SMOKE_L2, SMOKE_L3);
    /* �Ի��洫�������еȼ����� */
    lvl_flame = classify_flame(s_flame);
    lvl_light = (s_light <= 800U) ? 1 : 0;   /* ������Ԫ: ��800=�쳣 */
    if (s_temp_ok) {
        lt = classify((u16)s_temp, (u16)TEMP_L1, (u16)TEMP_L2, (u16)TEMP_L3);
        lr = (s_rate > 0) ? classify((u16)s_rate, (u16)RATE_L1, (u16)RATE_L2, (u16)RATE_L3) : 0;
        lvl_temp = (lt > lr) ? lt : lr;     /* �¶�+����ȡ�� */
    } else {
        lvl_temp = 0;     /* ������Ч������ */
    }

    /* ���س������У�˲ʱ����������ʱ�� */
    if (s_smoke > SMOKE_SEVERE && s_temp_ok && s_temp > TEMP_SEVERE) {
        target_level = ALARM_L3;
        return;
    }

    any_l3 = (lvl_smoke >= 3 || lvl_temp >= 3 || lvl_flame >= 3);
    any_l2 = (lvl_smoke >= 2 || lvl_temp >= 2 || lvl_flame >= 2);
    any_l1 = (lvl_smoke >= 1 || lvl_temp >= 1 || lvl_flame >= 1);

    /* ��+��+�� ����ͬʱ >= ���� */
    if (lvl_smoke >= 2 && lvl_temp >= 2 && lvl_flame >= 2) {
        target_level = ALARM_L3;
    }
    /* (��+��ͬʱ >= ����) �� ��һ���������� */
    else if ((lvl_smoke >= 2 && lvl_temp >= 2) || any_l3) {
        target_level = ALARM_L2;
    }
    /* ��һ������һ�� �� �޶��������� */
    else if (any_l1 && !any_l2) {
        target_level = ALARM_L1;
    }
    else {
        target_level = ALARM_NORMAL;
    }
}

/* ============================================================
 * ������ʱ������˲ʱ��һ�������� N ���ȶ�����
 * ���������� L1/L2 ���� 10s �� �Զ���һ��
 * ������target=0 ���� 10s �� �ָ�����
 * ============================================================ */
static void alarm_apply_stable(void)
{
    /* target �仯 �� ��������/������ʱ */
    if (target_level != prev_target) {
        level_ticks = 0;
        prev_target = target_level;
    }
    /* ����˲ʱ���� */
    if (target_level == ALARM_L3) {
        cur_level = ALARM_L3;
        stable_cnt = 0;
        level_ticks = 0;
        return;
    }

    /* �������Զ������� L3 ���������䣬���½� */
    if (cur_level == ALARM_L3 && target_level > ALARM_NORMAL && target_level < ALARM_L3) {
        level_ticks++;
        if (level_ticks >= ALARM_UPGRADE_TICKS) {
            cur_level--;
            level_ticks = 0;
        }
        return;
    }

    /* �������Զ������� L2 ���������䣬���½� */
    if (cur_level == ALARM_L2 && target_level > ALARM_NORMAL && target_level < ALARM_L2) {
        level_ticks++;
        if (level_ticks >= ALARM_UPGRADE_TICKS) {
            cur_level--;
            level_ticks = 0;
        }
        return;
    }

    /* һ���������� N ���ȶ����� */
    if (target_level == ALARM_L1 || target_level == ALARM_L2) {
        stable_cnt++;
        if (stable_cnt >= ALARM_STABLE_N && cur_level < target_level) {
            cur_level = target_level;
            level_ticks = 0;
        }
        /* ���������� L1/L2 ���� 10s �� �Զ���һ������ target δ����ʱ�� */
        if (cur_level > ALARM_NORMAL && cur_level < ALARM_L3 && target_level > cur_level) {
            level_ticks++;
            if (level_ticks >= ALARM_UPGRADE_TICKS) {
                cur_level++;
                level_ticks = 0;
            }
        }
        return;
    }
    /* target=0��10s �ȶ��󽵼� */
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
 * ִ�������ȣ��ȼ� �� ��������
 * �� enabled ʱ�·���ledstrip ֻ�ڵȼ��仯ʱ������������ blink��
 * ============================================================ */
static void alarm_dispatch_actuators(void)
{
    if (!enabled) return;     /* ����ģʽ�����ӹ�ִ���� */

    switch (cur_level) {
        case ALARM_NORMAL:
        if (!(manual_mask & ALARM_MANUAL_FAN))  { fan_set_speed(0); out_fan_pct = 0; }
        beep_period = 0;
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
        /* ����չ�㣺������ȼ�������������ʴ����������ƫ�͡��������� */
        if (!(manual_mask & ALARM_MANUAL_FAN))  { fan_set_speed(100); out_fan_pct = 100; }
        if (!(manual_mask & ALARM_MANUAL_PUMP)) { pump_on();        out_pump_on = 1; }
        /* ��������������drive_beep ���� */
        /* ����չ�㣺ledstrip ������Ƶ���������� mode 3 ������ */
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
 * beep ����/����������ÿ 10ms ����
 * ============================================================ */
static void alarm_drive_beep(void)
{
    if (!enabled) return;     /* ����ģʽ�����ӹ� beep */

    /* ����չ�㣺�����ڼ䣨mute_flag �ҷ����������� */
    if (mute_flag) {                    /* ����ʱ������ȫ�أ��� L3�� */
        if (beep_cur_on) { beep_off(); beep_cur_on = 0; }
        return;
    }

    if (cur_level == ALARM_NORMAL) {
        if (beep_cur_on) { beep_off(); beep_cur_on = 0; }
        beep_phase_cnt = 0;
        return;
    }
    if (cur_level == ALARM_L3) {          /* �������� */
        if (!beep_cur_on) { beep_on(); beep_cur_on = 1; }
        return;
    }
    /* L1/L2 ������������ toggle */
    beep_phase_cnt += 10;
    if (beep_phase_cnt >= beep_period / 2) {
        beep_phase_cnt = 0;
        beep_toggle();
        beep_cur_on = !beep_cur_on;
    }
}

/* ============================================================
 * ��ѭ����ڣ�ÿ 10ms��
 * ============================================================ */
void alarm_tick(void)
{
    tick_cnt++;

    if (enabled) {
        ledstrip_tick();  /* ͳһ�����ƴ� blink */
    }

    /* ÿ 2s���� DHT + ���������ʣ������ж�ǰ��ȷ����������/���ʣ� */
    if (tick_cnt % ALARM_TICK_DHT == 0) {
        alarm_sample_dht();
    }

    /* ÿ 100ms���ɼ� ADC + �ж� + ���� + ���� */
    if (tick_cnt % ALARM_TICK_SAMPLE == 0) {
        alarm_sample_adc();
        alarm_eval_levels();
        alarm_apply_stable();
        alarm_dispatch_actuators();
    }

    /* ÿ 10ms��beep �������� */
    alarm_drive_beep();
    if (mute_flag && mute_remain > 0) {
        if (--mute_remain == 0) mute_flag = 0;
    }

#ifdef ALARM_DEBUG_LOG
    /* ÿ 1s ��ӡһ��״̬ */
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
 * ��ʼ�� / ���� / ����
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
    alarm_sample_dht();      /* �״ζ���rate=0 */
    alarm_eval_levels();
    alarm_apply_stable();
    alarm_dispatch_actuators();
}

void alarm_enable(u8 on)    { enabled = on; if (on) { light_led_on = 0; last_disp_level = 0xFF; } }
u8   alarm_is_enabled(void) { return enabled; }

void alarm_mute(void)    { mute_flag = 1; mute_remain = 1500; }   /* ����չ�㣺15s ����ʱ����ʵ�� */
u8   alarm_is_muted(void){ return mute_flag; }

/* ============================================================
 * getter�����ݾۺ����ģ�
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
/* ====== �����ȼ� ====== */
u8   alarm_get_light_level(void)    { return lvl_light; }

/* ====== ִ�����ֶ����� ====== */
void alarm_manual_override(u8 mask) { manual_mask |= mask; }
void alarm_manual_release(u8 mask)  { manual_mask &= ~mask; }

/* ====== �𴫸���ԭʼֵ getter��view ��ʾ�ã� ====== */
u8   alarm_get_temp_n(u8 n)  { return (n == 1) ? s_t1 : s_t2; }
u8   alarm_get_humi_n(u8 n)  { return (n == 1) ? s_h1 : s_h2; }
u16  alarm_get_flame_n(u8 n) { return (n == 1) ? s_f1 : s_f2; }
u8   alarm_get_temp_n_ok(u8 n) { return (n == 1) ? s_ok1 : s_ok2; }

u8   alarm_get_flame_level_n(u8 n) { return classify_flame((n == 1) ? s_f1 : s_f2); }

