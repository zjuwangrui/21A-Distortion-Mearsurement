/**
 * App.tsx — THD 失真度测量装置 (21A) 手机端
 *
 * 架构：
 *   手机做 TCP HTTP 服务器 (port 8080)
 *   STM32 + ESP8266 (AP 模式) 每 ~1 s POST 一次 THD 数据
 *   手机在 HTTP 响应中回空 {}（不下发任何控制参数）
 *
 * 网络拓扑：
 *   ESP8266 AP: SSID = 21Athd-calculation   PWD = 12345678
 *   手机连上此热点后 IP 通常为 192.168.4.2
 *   STM32 代码 PHONE_IP 就填 192.168.4.2
 *
 * 收到格式:
 *   POST /thd  body:
 *   {"f0":1000.50,"thd":0.324,
 *    "h":[1.0000,0.0324,0.0152,0.0056,0.0031],
 *    "p":[0.00,1.57,3.14,-1.57,0.00]}
 */
import React, {useState, useCallback, useEffect} from 'react';
import {SafeAreaView, ScrollView, StyleSheet, Text, View} from 'react-native';
import {StatusBar} from 'expo-status-bar';
import * as Network from 'expo-network';

import {startServer, stopServer, ThdData} from './src/services/TcpServer';
import StatusLog        from './src/components/StatusLog';
import ConnectionInfo   from './src/components/ConnectionInfo';
import StatCard         from './src/components/StatCard';
import HarmonicsTable   from './src/components/HarmonicsTable';
import WaveformSvg      from './src/components/WaveformSvg';

const SERVER_PORT = 8080;
const AP_SSID     = '21Athd-calculation';
const MAX_LOGS    = 80;

export default function App() {
  const [thd, setThd]                   = useState<ThdData | null>(null);
  const [myIp, setMyIp]                 = useState('--');
  const [serverRunning, setServerRunning] = useState(false);
  const [stm32Connected, setStm32Connected] = useState(false);
  const [lastUpdate, setLastUpdate]     = useState<Date | null>(null);
  const [logs, setLogs]                 = useState<string[]>([]);

  const addLog = useCallback((msg: string) => {
    const t = new Date().toLocaleTimeString('zh-CN', {hour12: false});
    setLogs(prev => {
      const next = [...prev, `[${t}] ${msg}`];
      return next.length > MAX_LOGS ? next.slice(next.length - MAX_LOGS) : next;
    });
  }, []);

  /* 启动初始化：获取本机 IP，方便对照 STM32 端 PHONE_IP */
  useEffect(() => {
    addLog('系统启动');
    addLog(`需连接 WiFi: "${AP_SSID}"`);
    Network.getIpAddressAsync()
      .then(ip => {
        const addr = ip ?? '--';
        setMyIp(addr);
        addLog(`✓ 手机 IP: ${addr}`);
        addLog('点击"启动服务器"开始监听 STM32 数据');
      })
      .catch(() => {
        setMyIp('获取失败');
        addLog('✗ IP 获取失败，请检查 WiFi 连接');
      });
  }, [addLog]);

  /* STM32 每次上报都会走这里 */
  const handleThdData = useCallback((data: ThdData) => {
    setThd(data);
    setLastUpdate(new Date());
    const u1 = data.h[0] > 1e-6 ? data.h[0] : 1;
    const h2n = (data.h[1] / u1).toFixed(4);
    const h3n = (data.h[2] / u1).toFixed(4);
    addLog(`f0=${(data.f0 / 1000).toFixed(2)}kHz  THD=${data.thd.toFixed(3)}%  ` +
           `H2/H1=${h2n}  H3/H1=${h3n}`);
  }, [addLog]);

  const handleToggleServer = useCallback(() => {
    if (serverRunning) {
      stopServer();
      setServerRunning(false);
      setStm32Connected(false);
      addLog('--- 服务器已停止 ---');
    } else {
      addLog('正在启动 TCP 服务器...');
      startServer(SERVER_PORT, {
        onData: handleThdData,
        onLog: addLog,
        onClientConnect: addr => {
          setStm32Connected(true);
          addLog(`✓ STM32 已连接 (${addr})`);
        },
        onClientDisconnect: () => {
          /* STM32 每次 POST 都会断开重连，不用改状态 */
        },
      });
      setServerRunning(true);
    }
  }, [serverRunning, addLog, handleThdData]);

  const lastUpdateStr = lastUpdate
    ? `最近更新: ${lastUpdate.toLocaleTimeString('zh-CN', {hour12: false})}`
    : '等待数据...';

  return (
    <SafeAreaView style={styles.safe}>
      <StatusBar style="auto" />
      <ScrollView
        style={styles.scroll}
        contentContainerStyle={styles.content}
        keyboardShouldPersistTaps="handled">

        <Text style={styles.appTitle}>THD 失真度测量装置</Text>

        <ConnectionInfo
          myIp={myIp}
          serverPort={SERVER_PORT}
          serverRunning={serverRunning}
          stm32Connected={stm32Connected}
          wifiSsid={AP_SSID}
          onToggleServer={handleToggleServer}
        />

        <StatCard
          label="基频 f0"
          value={thd ? (thd.f0 / 1000).toFixed(2) : '--'}
          unit="kHz"
          subtext={lastUpdateStr}
        />

        <StatCard
          label="失真度 THD"
          value={thd ? thd.thd.toFixed(3) : '--'}
          unit="%"
          accent={thd && thd.thd > 5 ? '#F44336' : '#00C853'}
          subtext={lastUpdateStr}
        />

        <WaveformSvg
          h={thd ? thd.h : null}
          p={thd ? thd.p : null}
        />

        <HarmonicsTable h={thd ? thd.h : null} />

        <View style={styles.logCard}>
          <Text style={styles.logTitle}>调试日志</Text>
          <View style={styles.logBox}>
            <StatusLog logs={logs} />
          </View>
        </View>
      </ScrollView>
    </SafeAreaView>
  );
}

const styles = StyleSheet.create({
  safe:   {flex: 1, backgroundColor: '#ECEFF1'},
  scroll: {flex: 1},
  content:{padding: 12, paddingBottom: 32},
  appTitle: {
    fontSize: 20,
    fontWeight: 'bold',
    textAlign: 'center',
    marginVertical: 10,
    color: '#0D47A1',
    letterSpacing: 1,
  },
  logCard: {
    backgroundColor: '#fff',
    borderRadius: 10,
    padding: 14,
    elevation: 3,
  },
  logTitle: {fontSize: 13, color: '#888', marginBottom: 8},
  logBox: {height: 200},
});
