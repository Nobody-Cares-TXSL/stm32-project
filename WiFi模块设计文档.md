# WiFi 模块设计文档

> STM32F103RCT6 + ESP-01S（AT 固件）+ 电脑 Python 网页方案
> STM32 发送传感器数据 → ESP8266 WiFi 透传 → 电脑 Python 接收 + 网页展示

---

## 一、总体架构

```
STM32F103RCT6              ESP-01S (AT固件)            电脑
+------------------+      +------------------+      +------------------+
| USART2 TX (PA2) -+----->+ RX               |      | Python 脚本       |
| USART2 RX (PA3) <-+-----+ TX               |      |  - TCP Client     |
|                  |      | AP模式:"FireAlarm"| WiFi |  - Flask Server   |
| wifi.c           |      | TCP Server:8080  |<---->|  - HTML 网页      |
| sensor_data      |      +------------------+      |                  |
+------------------+                                | 浏览器访问        |
                                                    | localhost:5000    |
                                                    | 192.168.4.2:5000  |
                                                    +------------------+
```

### 数据流向

```
1. ESP8266 上电 → AT 指令配置 AP 热点 + TCP Server(端口 8080)
2. 电脑连接 WiFi "FireAlarm" → 运行 Python 脚本 → TCP 连接 192.168.4.1:8080
3. STM32 主循环每 100 tick(~1s) 读传感器 → USART2 发一帧数据
4. ESP8266 透传到 TCP → Python 收到 → 解析存入变量
5. 浏览器访问 localhost:5000 → Flask 返回 HTML + JS → 页面显示数据
6. JS 每 1 秒 fetch 更新数据（或页面 SSE 推送）
```

### 各端职责

| 端 | 职责 | 技术栈 |
|---|---|---|
| STM32 | 采集传感器数据，USART2 定时发送数据帧 | C, USART2, AT指令 |
| ESP8266 | WiFi AP 热点 + TCP Server 透传 | AT 固件(不改) |
| Python | TCP 客户端接收数据 + Flask Web Server | Python, Flask, Flask-SocketIO |
| 浏览器 | 显示实时数据仪表盘 | HTML + CSS + JavaScript |

---

## 二、STM32 侧设计

### 2.1 USART2 硬件

| 参数 | 值 |
|------|------|
| TX | PA2 (GPIO_Mode_AF_PP) |
| RX | PA3 (GPIO_Mode_IPU) |
| 波特率 | 115200 (ESP-01S 默认，若不通降 9600) |
| 格式 | 8N1 |
| 时钟 | APB1 (RCC_APB1Periph_USART2) |
| NVIC | USART2_IRQn, 抢占1 子1 |

### 2.2 USART2 驱动

参考 swap-space `usart.c` 模式，适配 USART2。

```c
/* ---- 接收 ---- */
#define WIFI_RX_BUF_LEN  256
char WIFI_RX_BUF[WIFI_RX_BUF_LEN];
volatile u8 WIFI_RX_STA = 0;      /* 1=一帧完成 */
static u16 WIFI_RX_num = 0;

void USART2_IRQHandler(void)
{
    u8 ch;
    if (USART_GetITStatus(USART2, USART_IT_RXNE)) {
        ch = USART_ReceiveData(USART2);
        if (WIFI_RX_num < WIFI_RX_BUF_LEN)
            WIFI_RX_BUF[WIFI_RX_num++] = ch;
        if (ch == '\n' || WIFI_RX_num >= WIFI_RX_BUF_LEN) {
            WIFI_RX_STA = 1;
            WIFI_RX_num = 0;
        }
        USART_ClearFlag(USART2, USART_IT_RXNE);
    }
}

/* ---- 发送 ---- */
void wifi_send_byte(u8 data)
{
    while (!USART_GetFlagStatus(USART2, USART_FLAG_TXE));
    USART_SendData(USART2, data);
}

void wifi_send_str(const char *s)
{
    while (*s) wifi_send_byte(*s++);
}

/* ---- 初始化 ---- */
void wifi_usart_init(void)
{
    /* 参考 swap-space usart.c 的 usart_scan()，改为 USART2 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
    GPIO_init(RCC_APB2Periph_TYPE_A, GPIO_Pin_2, GPIO_Mode_AF_PP, GPIOA);
    GPIO_init(RCC_APB2Periph_TYPE_A, GPIO_Pin_3, GPIO_Mode_IPU, GPIOA);
    /* USART_Init ... (同 swap-space 的 USART_init，改 USART1→USART2, baud=115200) */
    NVIC_init(USART2_IRQn, 1, 1);
}
```

### 2.3 AT 指令初始化序列

STM32 启动时通过 USART2 发送以下 AT 指令，让 ESP8266 开热点和 TCP Server。

| 步骤 | AT 指令 | 期望响应 | 说明 |
|------|---------|----------|------|
| 1 | `AT\r\n` | `OK` | 测试通信 |
| 2 | `ATE0\r\n` | `OK` | 关回显 |
| 3 | `AT+CWMODE=2\r\n` | `OK` | AP 模式 |
| 4 | `AT+CWSAP="FireAlarm","",1,0\r\n` | `OK` | 开热点(开放,频道1,encryption=0) |
| 5 | `AT+CIPMUX=1\r\n` | `OK` | 多连接模式 |
| 6 | `AT+CIPSERVER=1,8080\r\n` | `OK` | TCP Server 端口8080 |

初始化完成后 ESP8266 进入等待连接状态。
电脑上的 Python 脚本连接 192.168.4.1:8080 后，ESP8266 通知 STM32:
```
0,CONNECT
```
STM32 收到 "CONNECT" 后开始定时发送数据帧（通过 AT+CIPSEND 流程）。
收到 "CLOSED" 后停止发送（等重连）。

### 2.4 数据帧格式

STM32 发送的传感器数据帧，一行 ASCII 文本，`\n` 结尾：

```
T:25,H:60,S:150,F1:50,F2:45,L:800,A:0,FP:0,P:0\n
```

| 字段 | 含义 | 范围 | 说明 |
|------|------|------|------|
| T | 温度 | 0-99 | DHT11 读数 |
| H | 湿度 | 0-99 | DHT11 读数 |
| S | 烟雾 ADC | 0-4095 | MQ-2 AO |
| F1 | 火焰1 ADC | 0-4095 | KY-026 #1 |
| F2 | 火焰2 ADC | 0-4095 | KY-026 #2 |
| L | 光敏 ADC | 0-4095 | MH 光敏 |
| A | 报警等级 | 0-3 | 0=正常 1=预警 2=警报 3=火情 |
| FP | 风扇 PWM% | 0-100 | |
| P | 水泵 | 0/1 | 0=关 1=开 |

**优点**:
- 人类可读，方便调试（串口助手直接看）
- Python 一行 `split(',')` 即可解析
- 单帧约 40-50 字节，115200 下 < 0.5ms 发完

### 2.5 非阻塞 AT 状态机

```
WIFI_OFF ──AT──> WIFI_AT ──OK──> WIFI_ECHO ──OK──> WIFI_APMODE ──OK──>
  ^                  |                                           |
  |            timeout/error                             timeout/error
  |                  v                                           v
  |              retry x3                                   retry x3
  |                  |                                           |
  +──────────────────+─────────── 失败 → WIFI_OFF ──────────────┘

WIFI_APMODE ──OK──> WIFI_AP ──OK──> WIFI_MUX ──OK──> WIFI_SERVER ──OK──> WIFI_READY
                                                                         |
                                                               等待 "CONNECT"
                                                                    |
                                                               WIFI_CONNECTED
                                                                    |
                                                          定时走 CIPSEND 流程
```

每个 AT 指令拆成三阶段(非阻塞):

```
SEND(发指令) → WAIT(计数等响应) → CHECK(查 OK/ERROR/> / SEND OK)
```

每 tick(10ms) 调用 `wifi_tick()` 驱动状态机前进一步。

### 2.6 数据发送（AT+CIPSEND 流程）

**核心限制**: CIPSERVER 模式下不能透传，每发一帧数据必须走 CIPSEND 流程：

```
每秒发一帧数据的完整非阻塞流程:

WIFI_CONNECTED
  │
  ├─ SEND: 发 "AT+CIPSEND=0,50\r\n"     ← 50=数据帧字节数
  ├─ WAIT:  等 ">" 提示
  ├─ CHECK: 收到 ">" → 进入 SEND_DATA
  │
  ├─ SEND_DATA: 发 50 字节传感器数据     ← T:25,H:60,...\n
  ├─ WAIT:  等 "SEND OK"
  ├─ CHECK: 收到 "SEND OK" → 等待下一秒
  │         收到 "SEND FAIL" → 下一秒重试
  │         超时 → 标记发送失败，下一秒重试
  │
  └─ 收到 "CLOSED" → 回到 WIFI_READY（等重连）
```

```c
typedef enum {
    WIFI_OFF = 0,
    WIFI_AT,            /* 发 AT 测试通信 */
    WIFI_ECHO,          /* 发 ATE0 关回显 */
    WIFI_APMODE,        /* 发 AT+CWMODE=2 */
    WIFI_AP,            /* 发 AT+CWSAP */
    WIFI_MUX,           /* 发 AT+CIPMUX=1 */
    WIFI_SERVER,        /* 发 AT+CIPSERVER */
    WIFI_READY,         /* 就绪，等待 CONNECT */
    WIFI_CONNECTED,     /* 客户端已连接 */
    WIFI_CIPSEND,       /* 发 AT+CIPSEND 指令中 */
    WIFI_CIPSEND_WAIT,   /* 等待 ">" 提示 */
    WIFI_SEND_DATA,      /* 发送传感器数据中 */
    WIFI_SEND_WAIT       /* 等待 SEND OK */
} wifi_state_t;
```

```c
void wifi_tick(void)
{
    switch (wifi_state) {
    /* ... 初始化 AT 指令状态机（同前） ... */

    case WIFI_READY:
        /* 检查是否收到 "CONNECT" 或 "CLOSED" */
        if (WIFI_RX_STA) {
            if (strstr(WIFI_RX_BUF, "CONNECT"))
                wifi_state = WIFI_CONNECTED;
            WIFI_RX_STA = 0;
        }
        break;

    case WIFI_CONNECTED:
        /* 每 100 tick(~1s) 发一帧 */
        if (++send_tick >= 100) {
            send_tick = 0;
            wifi_state = WIFI_CIPSEND;
            at_phase = AT_SEND;
        }
        /* 同时检查 CLOSED */
        if (WIFI_RX_STA) {
            if (strstr(WIFI_RX_BUF, "CLOSED")) {
                wifi_state = WIFI_READY;
            }
            WIFI_RX_STA = 0;
        }
        break;

    case WIFI_CIPSEND:
        /* 三阶段: SEND → WAIT → CHECK */
        handle_at_phase("AT+CIPSEND=0,%d\r\n", frame_len);
        if (at_result == WIFI_PROMPT) {     /* 收到 ">" */
            wifi_state = WIFI_SEND_DATA;
            at_phase = AT_SEND;
        } else if (at_result == WIFI_ERROR || at_result == WIFI_TIMEOUT) {
            wifi_state = WIFI_CONNECTED;    /* 失败则跳过，下秒重试 */
        }
        break;

    case WIFI_SEND_DATA:
        /* 发送传感器数据帧 */
        wifi_send_sensor_frame();
        at_phase = AT_WAIT;
        wifi_state = WIFI_SEND_WAIT;
        break;

    case WIFI_SEND_WAIT:
        /* 等待 SEND OK */
        if (++at_timeout >= 100) {           /* 1s 超时 */
            wifi_state = WIFI_CONNECTED;
        } else if (WIFI_RX_STA) {
            if (strstr(WIFI_RX_BUF, "SEND OK"))
                wifi_state = WIFI_CONNECTED;
            /* SEND FAIL 或超时 → 下秒重试 */
            wifi_state = WIFI_CONNECTED;
            WIFI_RX_STA = 0;
        }
        break;
    }
}
```

```c
void wifi_send_sensor_frame(void)
{
    char buf[64];
    sprintf(buf, "T:%d,H:%d,S:%d,F1:%d,F2:%d,L:%d,A:%d,FP:%d,P:%d\n",
            temp, humi, smoke_adc, flame1_adc, flame2_adc,
            light_adc, alarm_level, fan_pwm, pump_on);
    wifi_send_str(buf);
}
```

### 2.7 ESP8266 启动延迟处理

ESP8266 上电后需要 **2-3 秒** 才能接受 AT 指令。
STM32 启动后立刻发 `AT` 会超时，需要等待后再开始初始化：

```c
/* wifi_tick() 中 WIFI_OFF 状态 */
case WIFI_OFF:
    if (++wait_ticks >= 300) {     /* 300 tick = 3s，等 ESP8266 启动 */
        wait_ticks = 0;
        retry = 0;
        wifi_state = WIFI_AT;
    }
    break;
```

首次上电完整初始化时间线：等 3s + 最多 6 步 AT × (2s 超时 × 3 次重试) ≈ 最坏 39s，
正常情况（全部 OK）≈ 5s。

### 2.8 波特率自适应

ESP-01S 出厂可能是 9600 或 115200。WIFI_OFF 状态先尝试 115200，
失败 3 轮后自动切换 9600 重试：

```c
static u32 wifi_baud = 115200;

case WIFI_OFF:
    if (++wait_ticks >= 300) {
        wait_ticks = 0;
        retry = 0;
        /* 切换波特率 */
        wifi_set_baud(wifi_baud);
        wifi_state = WIFI_AT;
    }
    break;

/* AT 步骤: 3 次 OK 都失败 */
case WIFI_AT:
    /* ... 正常逻辑 ... */
    if (retry >= 3) {
        if (wifi_baud == 115200) {
            wifi_baud = 9600;       /* 降速重试 */
        } else {
            wifi_baud = 115200;     /* 下次从 115200 开始 */
        }
        wifi_state = WIFI_OFF;
    }
    break;
```

`wifi_set_baud()` 需要关闭 USART2 → 重配 USART_Init baudrate → 重新使能。

### 2.9 WiFi 视图(菜单)

标准三件套 `wifi_view_enter/tick/on_event`。

视图布局(128x128 LCD):
```
+------------------+
| WiFi             |
|                  |
| AP: FireAlarm    |
| Port: 8080       |
|                  |
| Status: CONNECT   |  绿=CONNECTED 红=OFF 黄=初始化中
| Client: YES/NO   |
|                  |
| DN: BACK         |
+------------------+
```

---

## 三、ESP8266 侧(不改固件)

### 3.1 硬件接线

| ESP-01S | 到 STM32 / 电源 | 说明 |
|---------|----------------|------|
| 3V3 | 电源 3V3 | 电源正 |
| GND | GND | 电源地 |
| TX | PA3 | USART2_RX |
| RX | PA2 | USART2_TX |
| EN(CH_PD) | 3V3 | 使能，必须高电平（内部无上拉） |
| RST | 3V3 | 复位，高=正常 / 拉低=复位 |
| IO0 | 3V3 | 低=刷固件模式，必须高=正常启动 |
| IO2 | 3V3 | 必须高电平（低=刷固件模式） |

> **电平**: ESP-01S 与 STM32F103 均为 3.3V，TX/RX 直连无需电平转换。
> **供电**: ESP-01S 发射峰值 250-300mA，务必确认 3V3 电源（LDO）余量；发送瞬间掉电会导致模块反复重启、AT 无响应。

### 3.2 工作模式

只需一次性通过 STM32 发 AT 指令配置，之后 ESP8266 自动运行:

1. AP 热点 "FireAlarm"（开放，无密码）
2. IP: 192.168.4.1
3. TCP Server 监听 8080 端口
4. 电脑连接后透传数据

**注意**: AT+CIPSERVER 模式下 ESP8266 **不是透传**。STM32 必须通过
`AT+CIPSEND=ch,len` → 等待 `>` → 发送数据 → 等待 `SEND OK` 的流程
来向客户端发送数据（详见 2.6 节）。
不能使用 `AT+CIPMODE=1`（透传模式），因为 CIPSERVER 要求 CIPMUX=1，
而 CIPMODE=1 要求 CIPMUX=0，两者互斥。

---

## 四、电脑端 Python 程序设计

### 4.1 整体结构

```
wifi_monitor.py          单文件 Python 脚本
├── TCP Client          连接 192.168.4.1:8080，接收传感器数据
├── Flask Server        本地 Web Server (localhost:5000)
├── SSE/REST API        向前端推送或提供数据接口
└── HTML 模板           内嵌 HTML 网页(或 templates/index.html)
```

### 4.2 技术选型

| 组件 | 方案 | 理由 |
|------|------|------|
| Web 框架 | Flask | 轻量，单文件可写 |
| 实时推送 | Server-Sent Events (SSE) | 比 WebSocket 简单，浏览器原生支持 |
| TCP 客户端 | socket 线程 | 后台线程持续接收 |
| 前端 | 纯 HTML+CSS+JS | 无需构建工具，打开即用 |

### 4.3 Python 脚本架构

```python
# wifi_monitor.py

import socket
import threading
from flask import Flask, render_template_string
import json, time

# ====== 全局数据(线程安全) ======
data_lock = threading.Lock()
sensor_data = {
    "temp": 0, "humi": 0, "smoke": 0,
    "flame1": 0, "flame2": 0, "light": 0,
    "alarm": 0, "fan_pwm": 0, "pump": 0,
    "connected": False, "last_update": 0
}

# ====== TCP 客户端线程 ======
def tcp_client():
    """后台线程: 连接 ESP8266 TCP Server，持续接收数据帧"""
    while True:
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.connect(("192.168.4.1", 8080))
            sensor_data["connected"] = True
            buf = ""
            while True:
                data = sock.recv(512).decode()
                if not data:
                    break
                buf += data
                while "\n" in buf:
                    line, buf = buf.split("\n", 1)
                    if line:
                        parse_frame(line)
        except (ConnectionRefusedError, OSError):
            sensor_data["connected"] = False
        time.sleep(5)  # 断开后 5 秒重连

def parse_frame(line):
    """解析数据帧: T:25,H:60,S:150,F1:50,F2:45,L:800,A:0,FP:0,P:0"""
    pairs = line.split(",")
    with data_lock:
        for p in pairs:
            if ":" not in p:
                continue
            k, v = p.split(":", 1)
            mapping = {
                "T": "temp", "H": "humi", "S": "smoke",
                "F1": "flame1", "F2": "flame2", "L": "light",
                "A": "alarm", "FP": "fan_pwm", "P": "pump"
            }
            if k in mapping:
                sensor_data[mapping[k]] = int(v)
        sensor_data["last_update"] = time.time()

# ====== Flask Web Server ======
app = Flask(__name__)

HTML_TEMPLATE = """
<!DOCTYPE html>
<html><head>
<meta charset="utf-8">
<title>Fire Alarm Monitor</title>
<style>
  body { font-family:Arial; background:#1a1a2e; color:#e0e0e0; margin:0; padding:20px; }
  h1 { text-align:center; color:#e94560; }
  .grid { display:grid; grid-template-columns:repeat(auto-fit,minmax(200px,1fr)); gap:12px; }
  .card { background:#16213e; padding:16px; border-radius:10px; border-left:4px solid #0f3460; }
  .label { font-size:12px; color:#888; text-transform:uppercase; }
  .val { font-size:28px; font-weight:bold; margin-top:4px; }
  .status { text-align:center; padding:16px; border-radius:10px;
            margin-top:16px; font-size:20px; font-weight:bold; }
</style>
</head><body>
<h1>Fire Alarm Monitor</h1>
<div class="grid">
  <div class="card"><div class="label">TEMP</div><div class="val" id="v-temp">-- C</div></div>
  <div class="card"><div class="label">HUMI</div><div class="val" id="v-humi">-- %</div></div>
  <div class="card"><div class="label">SMOKE</div><div class="val" id="v-smoke">--</div></div>
  <div class="card"><div class="label">FLAME 1</div><div class="val" id="v-flame1">--</div></div>
  <div class="card"><div class="label">FLAME 2</div><div class="val" id="v-flame2">--</div></div>
  <div class="card"><div class="label">LIGHT</div><div class="val" id="v-light">--</div></div>
</div>
<div class="status" id="v-status" style="background:#4ecca3">NORMAL</div>
<p style="text-align:center;color:#666;font-size:12px">
  WiFi: <span id="v-wifi">DISCONNECTED</span>
</p>
<script>
function update() {
  fetch("/api/data")
    .then(r => r.json())
    .then(d => {
      document.getElementById("v-temp").textContent   = d.temp + " C";
      document.getElementById("v-humi").textContent   = d.humi + " %";
      document.getElementById("v-smoke").textContent  = d.smoke;
      document.getElementById("v-flame1").textContent = d.flame1;
      document.getElementById("v-flame2").textContent = d.flame2;
      document.getElementById("v-light").textContent  = d.light;
      var names = ["NORMAL","WARNING","ALERT","FIRE"];
      var colors = ["#4ecca3","#f0a500","#e94560","#ff0000"];
      document.getElementById("v-status").textContent = names[d.alarm];
      document.getElementById("v-status").style.background = colors[d.alarm];
      document.getElementById("v-wifi").textContent = d.connected ? "CONNECTED" : "DISCONNECTED";
    });
}
setInterval(update, 1000);  /* 每秒刷新 */
update();
</script>
</body></html>
"""

@app.route("/")
def index():
    return render_template_string(HTML_TEMPLATE)

@app.route("/api/data")
def api_data():
    with data_lock:
        return json.dumps(sensor_data)

# ====== 启动 ======
if __name__ == "__main__":
    t = threading.Thread(target=tcp_client, daemon=True)
    t.start()
    print("Server: http://localhost:5000")
    print("Connect WiFi: FireAlarm (open, no password)")
    print("TCP: 192.168.4.1:8080")
    app.run(host="0.0.0.0", port=5000)
```

### 4.4 使用方法

```bash
# 1. 电脑连接 WiFi "FireAlarm"
# 2. 运行 Python 脚本
python3 wifi_monitor.py

# 3. 浏览器打开
http://localhost:5000
# 或局域网内其他设备
http://192.168.4.2:5000
```

### 4.5 网页效果

```
+------------------------------------------+
|        Fire Alarm Monitor                |
+------------------------------------------+
|                                          |
|  +--------+  +--------+  +--------+      |
|  | TEMP   |  | HUMI   |  | SMOKE  |      |
|  | 25 C   |  | 60 %   |  | 150    |      |
|  +--------+  +--------+  +--------+      |
|                                          |
|  +--------+  +--------+  +--------+      |
|  | FLAME1 |  | FLAME2 |  | LIGHT  |      |
|  | 50     |  | 45     |  | 800    |      |
|  +--------+  +--------+  +--------+      |
|                                          |
|  +--------------------------------------+|
|  |         NORMAL (绿色背景)             ||
|  +--------------------------------------+|
|           WiFi: CONNECTED                 |
+------------------------------------------+
```

- JS 每秒 fetch `/api/data` 更新
- 报警等级: 正常=绿 预警=黄 警报=红 火情=红色
- 显示 WiFi 连接状态
- 响应式布局，手机可用

---

## 五、STM32 文件结构

```
code/src/usart.c         USART1 调试 log（printf 重定向，搬自 swap-space）
code/inc/usart.h         对外接口
code/src/wifi.c          USART2 驱动 + AT 状态机 + 数据发送
code/inc/wifi.h          对外接口
code/src/wifi_view.c     WiFi 菜单视图
code/inc/wifi_view.h     视图头文件
```

### wifi.h 接口

```c
#ifndef __WIFI_H
#define __WIFI_H
#include "stm32f10x.h"

void wifi_init(void);       /* USART2 初始化 + 启动 AT 状态机 */
void wifi_tick(void);       /* 每 tick(10ms) 调用 */

u8  wifi_is_ready(void);    /* WiFi 是否就绪(热点已开) */
u8  wifi_has_client(void);  /* 电脑是否已连接 TCP */

#endif
```

### 菜单挂载

```c
/* menu.c: 加一行 */
#include "wifi_view.h"
{ "WiFi", wifi_view_enter, wifi_view_tick, wifi_view_on_event },
```

### main.c

```c
#include "usart.h"   /* USART1 调试 log */
#include "wifi.h"
/* beep_init() 之后 */
usart_scan();        /* 先初始化 USART1，开 printf（需 Keil 勾 Use MicroLIB） */
wifi_init();
```

---

## 六、性能分析

### STM32 侧

| 项目 | 大小 |
|------|------|
| WIFI_RX_BUF | 256 字节 |
| AT 状态机代码 | ~150 行 |
| CIPSEND 流程代码 | ~100 行 |
| USART2 驱动代码 | ~80 行 |
| 数据发送代码 | ~30 行 |
| 波特率自适应 | ~30 行 |
| **RAM 合计** | **~300 字节** |
| **Flash 合计** | **~5 KB** |

每秒发送一帧 ~50 字节 @ 115200 ≈ 0.4ms，AT+CIPSEND 额外开销 ~2ms，
总计 ~2.5ms/秒，对主循环无影响。

### CPU 开销

- USART2 中断: 每次 1 字节，微秒级
- AT 状态机 tick: 仅检查标志位
- CIPSEND 流程: 每 1 秒触发一次，非阻塞，~2.5ms
- **主循环无阻塞**

---

## 七、错误处理

| 场景 | 检测 | 处理 |
|------|------|------|
| ESP8266 未上电 | AT 无响应 | 重试 3 次 → WIFI_OFF，每 30s 重试 |
| AT 返回 ERROR | 解析响应 | 重试当前步骤 |
| ESP8266 返回 busy | 解析响应 | 延迟 2s 重试 |
| 波特率不匹配 | AT 全超时 | 可选: 115200→9600 自动切换 |
| 电脑断开 TCP | 收到 "CLOSED" | 停发数据，等重连 |
| 缓冲溢出 | WIFI_RX_BUF 满 | 丢弃当前帧 |
| WiFi 断开不影响本地 | 独立状态机 | 安全功能照常运行 |

---

## 八、调试步骤

> **引脚占用已核实**（代码实测）：摇杆 SW 在 PC5（`joystick.c`），PA2/PA3 全空闲，USART2 可直接占用，与现有外设无冲突。

### 8.1 双串口架构（关键认知）

STM32 用两路独立串口，职责分明：

| 串口 | 引脚 | 接谁 | 用途 | 波特率 |
|------|------|------|------|--------|
| **USART2** | PA2/PA3 | ESP-01S | AT 指令 + 传感器数据（正经通道） | 115200 |
| **USART1** | PA9/PA10 | USB → 电脑 | `printf` 调试 log（旁路观察） | 9600 |

```
电脑（两个独立接口，互不冲突）
  ├─ WiFi 网卡 ──无线──> 连 "FireAlarm" 热点 ──> 收 ESP 数据（Python 那路）
  └─ USB ──有线──> STM32 USART1(PA9/PA10) ──> 看 printf log
```

> **无需第二个 USB-TTL**：电脑用 WiFi 网卡接收数据、用 USB 看串口 log，二者独立。
> 因此**跳过"纯电脑直连 ESP 手动测 AT"**，直接 STM32 + ESP 合体，全靠 USART1 log 调试。

### 8.2 搭建 USART1 调试 log（第一步，必须先做）

直接搬 swap-space 的 `usart.c/.h`（接口与本项目兼容，都用 `io.h` 的 `GPIO_init`/`NVIC_init`）：

- `PA9(TX)/PA10(RX)`、9600、`fputc` 重定向（支持 `printf`）
- 自带 RX 中断（`USART1_RX_BUF`/`USART1_RX_STA`），还能反向收电脑串口助手指令
- `usart_scan()` 一键初始化

> ⚠️ **Keil 必须勾选 Use MicroLIB**（Options → Target → Code Generation → Use MicroLIB），否则 `printf` 卡死或编译报错。

**验证**：`main.c` 加 `usart_scan(); printf("hello\r\n");`，串口助手（9600）收到 `hello` = log 通道通了，后面 wifi.c 才有"眼睛"。

### 8.3 wifi.c 调试 log 策略

无法独立测 ESP，所以 wifi.c 必须**超详细 log**，把 STM32↔ESP 交互全部摊开：

- 每发一条 AT：`printf("[TX] AT+CWMODE=2\r\n");`
- 每次 RX 中断收到数据：`printf("[RX] %s", WIFI_RX_BUF);`
- 状态机跳转：`printf("[ST] ->WIFI_CONNECTED\r\n");`

这样 ESP 没反应时，log 能一眼定位（无 `[RX]` = 硬件问题；有 `[RX]` 但内容异常 = 时序/解析问题）。

### 8.4 调试流程

```
第一步: 搭 USART1 log（见 8.2）
  - 搬 usart.c/.h，main.c 调 usart_scan()
  - printf("hello") 能在串口助手看到 → log 通

第二步: ESP 接 USART2 + 上电（等 3s）
  - 按 3.1 接线表接好
  - STM32 发 "AT\r\n"，log 应见 [TX]AT 和 [RX]OK
  - 若无 [RX]: 按 8.5 排查清单查硬件

第三步: AP 热点验证
  - 发 AT+CWMODE=2 / AT+CWSAP
  - 电脑搜 WiFi，确认 "FireAlarm" 出现

第四步: TCP Server + 数据发送
  - 发 AT+CIPMUX=1 / AT+CIPSERVER=1,8080
  - 电脑连 "FireAlarm"，运行 wifi_monitor.py
  - 确认收到 "T:25,H:60,..."

第五步: 网页展示
  - 浏览器 localhost:5000，确认数据实时更新
```

### 8.5 ESP 无响应排查清单

没有条件独立测 ESP，万一 `AT` 无响应，按序查：

| # | 检查项 | 现象/后果 |
|---|--------|----------|
| 1 | `EN(CH_PD)` 接 3V3 | 缺了模块根本不启动，AT 完全无响应 |
| 2 | `IO0`、`IO2` 接 3V3 | 拉低进刷机模式，不响应 AT |
| 3 | `RST` 接 3V3 | 拉低 = 一直复位 |
| 4 | TX/RX 接反 | **ESP-TX→PA3**、**ESP-RX→PA2**（交叉接） |
| 5 | 3V3 供电稳 | 发射瞬间 250mA，LDO 顶不住反复重启，log 表现为 AT 时通时不通 |
| 6 | 波特率 | 115200 全超时 → 降 9600（2.8 节自适应会自动切） |

---

## 九、与功能联动文档对照

| 功能联动文档要求 | 本设计实现 |
|------------------|-----------|
| USART2 PA2/PA3 | PA2 TX, PA3 RX, 115200 |
| 每 5 秒上报 | STM32 每 1 秒发帧(可调)，Python 即时转发 |
| 事件即时上报 | alarm_level 变化立即体现在下一帧 |
| 上报字段(温度/湿度/烟雾/火焰/光敏/风扇/水泵) | 数据帧全部覆盖 |
| 断线不影响本地 | WIFI_OFF 状态不阻塞主循环 |
| 断线缓存补发(30分钟) | 本地 AP 不需要(电脑重连即收最新数据) |
| OLED 显示 WiFi 图标 | WiFi 视图显示状态 |

---

## 十、实现步骤

| 步骤 | 内容 | 文件 | 代码量 |
|------|------|------|--------|
| 1 | USART2 驱动(初始化+中断+收发) | wifi.c/.h | ~80 行 |
| 2 | AT 引擎(非阻塞状态机) | wifi.c | ~150 行 |
| 3 | AT+CIPSEND 发送流程 | wifi.c | ~100 行 |
| 4 | 数据帧格式化(定时) | wifi.c | ~30 行 |
| 5 | 波特率自适应 | wifi.c | ~30 行 |
| 6 | WiFi 视图(菜单) | wifi_view.c/.h | ~80 行 |
| 7 | 菜单挂载 + main.c 初始化 | menu.c / main.c | ~10 行 |
| **STM32 合计** | | | **~480 行** |
| 8 | Python 网页监控脚本 | wifi_monitor.py | ~160 行 |
| **总计** | | | **~640 行** |

---

## 十一、数据采集与报警等级（wifi.c 前置依赖）

`wifi_send_sensor_frame()` 中的 temp/humi/smoke_adc 等变量需从各模块取。逐项核对接口可用性：

| 帧字段 | 取数方式 | 现状 |
|--------|---------|------|
| S 烟雾 | `smoke_read_ao()` | 直接可用 |
| F1/F2 火焰 | `flame_read_ao(FLAME_SENSOR_1)` / `flame_read_ao(FLAME_SENSOR_2)` | 直接可用 |
| L 光敏 | `light_read_ao()` | 直接可用 |
| FP 风扇 | `fan_get_speed()` | 直接可用 |
| P 水泵 | `pump_is_on()` | 直接可用 |
| T/H 温湿度 | `dht_read(port,pin,&t,&h)` | ⚠️ 阻塞 ~20ms/路，两路共 40ms |
| A 报警等级 | 无现成接口 | ❌ 需新建 |

### 温湿度阻塞处理（必须改 dht.c）

`dht_read` 是阻塞单总线读取。wifi 每秒在格式化数据时调用，主循环会卡顿 40ms（LCD/菜单/摇杆明显顿）。
方案：DHT 视图负责节流读取并缓存，wifi 读缓存值。需在 `dht.c/.h` 增加非阻塞 getter：

```c
/* dht.h 新增 */
void dht_cache_update(void);   /* DHT 视图 tick 节流调用，读两路并缓存 */
u8   dht_get_temp(u8 idx);     /* idx 0/1，读缓存，非阻塞 */
u8   dht_get_humi(u8 idx);
```

帧中 T/H 取哪一路需确定：建议两路都发（拆成 T1/H1/T2/H2），或只发主传感器。

### 报警等级 A 判定（必须定义）

现有各传感器只有 `xxx_read_alarm()` 返回 0/1（二值）。帧里 A:0-3 分级项目内不存在，需新建判定逻辑（放 `wifi.c` 或独立 `alarm.c`）：

| 等级 | 含义 | 判定条件（建议阈值，烧录后标定） |
|------|------|-------------------------------|
| 0 | 正常 | 无任何报警 |
| 1 | 预警 | 烟雾 AO > S1 或 火焰 AO 任一 > F1 |
| 2 | 警报 | 烟雾 > S2 或 火焰 > F2 |
| 3 | 火情 | 烟雾高 且 火焰高 |

阈值 S1/S2/F1/F2 待硬件标定后填入。
