#include "th_view.h"
#include "dht.h"
#include "lcd.h"        /* LCD_Fill / LCD_ShowChinese / LCD_ShowString + BLACK/WHITE */
#include "joystick.h"   /* JOY_DOWN */
#include <stdio.h>      /* sprintf */
#include <string.h>     /* strlen */

/* ====== 两颗传感器引脚（换线/增减传感器只改这里） ====== */
#define TH_PORT1  GPIOB
#define TH_PIN1   GPIO_Pin_14   /* 传感器1：短线 */
#define TH_PORT2  GPIOB
#define TH_PIN2   GPIO_Pin_15   /* 传感器2：长线 */

/* ====== 刷新节流：主循环约10ms，100次≈1秒（DHT11 采样间隔下限） ====== */
#define TH_REFRESH_TICKS  100

/* ====== 布局（128×128，16px/行） ====== */
#define TH_Y_TITLE   0     /* 标题：温湿度模块 */
#define TH_Y_LABEL1  24    /* 传感器1 标签 */
#define TH_Y_VALUE1  42    /* 传感器1 数值 */
#define TH_Y_LABEL2  66    /* 传感器2 标签 */
#define TH_Y_VALUE2  84    /* 传感器2 数值 */
#define TH_Y_HINT    110   /* 返回提示（ASCII，字库无"返回"二字） */

/* ====== 数值缓存（含读取是否成功） ====== */
static u8 th_t1 = 0, th_h1 = 0, th_ok1 = 0;   /* 传感器1 */
static u8 th_t2 = 0, th_h2 = 0, th_ok2 = 0;   /* 传感器2 */
static u16 th_tick = 0;                        /* 刷新计数器 */

/* 画"中文标签:数值"，返回结束 x（中文每字16px，ASCII每字8px） */
static u16 show_cn_num(u16 x, u16 y, const u8 *cn, u8 val, u8 ok)
{
    char buf[8];
    u16 len = 0;
    while (cn[len] != 0) len++;                 /* GB2312 字节数（每字2字节） */
    LCD_ShowChinese(x, y, (u8 *)cn, WHITE, BLACK, 16, 0);
    x += (len / 2) * 16;                        /* 中文标签宽度 */
    if (ok) sprintf(buf, ":%d", val);
    else    sprintf(buf, ":--");                /* 读取失败/未接 */
    LCD_ShowString(x, y, (u8 *)buf, WHITE, BLACK, 16, 0);
    x += (u16)strlen(buf) * 8;                  /* ASCII数值宽度 */
    return x;
}

/* 画一行数值：温度+湿度（先清行避免残留） */
static void th_draw_value(u16 y, u8 t, u8 h, u8 ok)
{
    u16 x;
    LCD_Fill(0, y, 128, y + 16, BLACK);
    x = show_cn_num(2, y, (const u8 *)"温度", t, ok);
    show_cn_num(x + 4, y, (const u8 *)"湿度", h, ok);
}

void th_view_enter(void)
{
    th_ok1 = 0;                                 /* 进入时数据未知，先显示 -- */
    th_ok2 = 0;
    th_tick = TH_REFRESH_TICKS;                 /* 让首次 tick 立即读一次 */

    LCD_Fill(0, 0, 128, 128, BLACK);
    LCD_ShowChinese(0, TH_Y_TITLE, (u8 *)"温湿度模块", WHITE, BLACK, 16, 0);

    LCD_ShowChinese(0, TH_Y_LABEL1, (u8 *)"传感器", WHITE, BLACK, 16, 0);
    LCD_ShowString(48, TH_Y_LABEL1, (u8 *)"1", WHITE, BLACK, 16, 0);
    LCD_ShowChinese(0, TH_Y_LABEL2, (u8 *)"传感器", WHITE, BLACK, 16, 0);
    LCD_ShowString(48, TH_Y_LABEL2, (u8 *)"2", WHITE, BLACK, 16, 0);

    LCD_ShowString(0, TH_Y_HINT, (u8 *)"DOWN:BACK", WHITE, BLACK, 16, 0);

    th_draw_value(TH_Y_VALUE1, 0, 0, 0);
    th_draw_value(TH_Y_VALUE2, 0, 0, 0);
}

void th_view_tick(void)
{
    u8 t, h;

    if (++th_tick < TH_REFRESH_TICKS) return;   /* 节流：约1秒读一次 */
    th_tick = 0;

    if (dht_read(TH_PORT1, TH_PIN1, &t, &h)) { th_t1 = t; th_h1 = h; th_ok1 = 1; }
    else th_ok1 = 0;
    if (dht_read(TH_PORT2, TH_PIN2, &t, &h)) { th_t2 = t; th_h2 = h; th_ok2 = 1; }
    else th_ok2 = 0;

    th_draw_value(TH_Y_VALUE1, th_t1, th_h1, th_ok1);
    th_draw_value(TH_Y_VALUE2, th_t2, th_h2, th_ok2);
}

u8 th_view_on_event(u8 ev, u8 sw)
{
    if (ev == JOY_DOWN) return 1;               /* 下拨 → 返回主菜单 */
    return 0;
}
