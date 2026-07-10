# THD Meter App (21A 电赛)

STM32F103 + ESP8266 采样并算 THD → WiFi → 手机 App 实时显示。

## 网络拓扑

```
STM32F103 ── UART3 ── ESP8266 (SoftAP)
                         │
                         │  WiFi
                         ▼
                     手机 (Expo/React Native app)
                     TCP server :8080
```

- WiFi 名: `21Athd-calculation`
- 密码: `12345678`
- 手机 IP: 通常是 `192.168.4.2`（连上热点后自动分配）
- STM32 侧 `include/drv/esp8266.h` 里的 `PHONE_IP` 要跟这个一致

## 通信协议

STM32 每 1 秒向手机 POST 一次：

```
POST /thd HTTP/1.0
Host: 192.168.4.2
Content-Type: application/json
Content-Length: ...

{"f0":1000.50,"thd":0.324,
 "h":[1.0000,0.0324,0.0152,0.0056,0.0031],
 "p":[0.00,1.57,3.14,-1.57,0.00]}
```

- `f0`：基频 Hz
- `thd`：失真度 %
- `h[5]`：H1..H5 幅度（未归一化，前端算 h[k]/h[0] 得 Uk/U1）
- `p[5]`：H1..H5 相位（弧度，用于波形重建）

手机响应：
```
HTTP/1.0 200 OK
Content-Length: 2

{}
```

不下发任何控制参数。

## 界面

- **连接信息**：WiFi SSID / 手机 IP / 启动服务器按钮 / STM32 连接状态
- **StatCard × 2**：基频 f0、失真度 THD（大数字）
- **WaveformSvg**：一个基频周期的重建波形（SVG polyline）
- **HarmonicsTable**：H1..H5 归一化幅值表
- **调试日志**：底部深色终端风格滚动区

## 目录

```
app/
├── App.tsx                              # 主界面
├── src/
│   ├── components/
│   │   ├── ConnectionInfo.tsx           # 网络状态 + 启停按钮
│   │   ├── StatCard.tsx                 # 通用大数字卡（f0 / THD 各一个）
│   │   ├── WaveformSvg.tsx              # 一周期重建波形（SVG）
│   │   ├── HarmonicsTable.tsx           # Uk/U1 表格
│   │   └── StatusLog.tsx                # 调试日志滚动区
│   └── services/
│       └── TcpServer.ts                 # 8080 端口 TCP 服务器
├── app.json
└── package.json
```

## 运行

```bash
cd app
npm install       # 首次
npm start         # 启动 Expo Dev Server
# 用 Expo Go 扫码，或 npm run android 直接跑到 Android 设备
```

**注意**：`react-native-tcp-socket` 不是纯 JS，Expo Go 里跑不了，需要 `expo prebuild` + `expo run:android`（或 EAS Build）。

## 常见问题

- **手机 App 一直"未连接"**：先确认已连 `21Athd-calculation` WiFi；再点"启动服务器"；然后检查 STM32 那边 `PHONE_IP` 是否等于本机 IP。
- **手机 IP 不是 192.168.4.2**：不同厂商手机分配 IP 不一定；把 App 上显示的手机 IP 抄到 STM32 的 `include/drv/esp8266.h` 中 `PHONE_IP` 宏，重烧。
- **数据一直 --**：STM32 没上报。检查：ESP_Init 是否成功（USART1 打印会说）、STM32 是否连上了 AP、路由是否正常。
