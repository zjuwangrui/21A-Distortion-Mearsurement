import React from 'react';
import {View, Text, StyleSheet, TouchableOpacity} from 'react-native';

type Props = {
  myIp: string;
  serverPort: number;
  serverRunning: boolean;
  stm32Connected: boolean;
  wifiSsid: string;
  onToggleServer: () => void;
};

type StatusDotProps = {color: string; label: string};

function StatusDot({color, label}: StatusDotProps) {
  return (
    <View style={styles.dotRow}>
      <View style={[styles.dot, {backgroundColor: color}]} />
      <Text style={styles.dotLabel}>{label}</Text>
    </View>
  );
}

export default function ConnectionInfo({
  myIp,
  serverPort,
  serverRunning,
  stm32Connected,
  wifiSsid,
  onToggleServer,
}: Props) {
  return (
    <View style={styles.card}>
      <Text style={styles.title}>连接信息</Text>

      <View style={styles.infoGrid}>
        <InfoRow label="WiFi" value={wifiSsid} note="(需连接ESP8266热点)" />
        <InfoRow label="手机IP" value={myIp} />
        <InfoRow label="监听端口" value={String(serverPort)} />
        <InfoRow
          label="STM32目标IP"
          value="192.168.4.2 → 本机"
          note="STM32代码中的PHONE_IP"
        />
      </View>

      <View style={styles.statusRow}>
        <StatusDot
          color={serverRunning ? '#4CAF50' : '#9E9E9E'}
          label={`HTTP服务器 ${serverRunning ? '运行中' : '未启动'}`}
        />
        <StatusDot
          color={stm32Connected ? '#4CAF50' : '#9E9E9E'}
          label={`STM32 ${stm32Connected ? '已连接' : '未连接'}`}
        />
      </View>

      <TouchableOpacity
        style={[
          styles.btn,
          {backgroundColor: serverRunning ? '#D32F2F' : '#1565C0'},
        ]}
        onPress={onToggleServer}>
        <Text style={styles.btnText}>
          {serverRunning ? '停止服务器' : '启动服务器'}
        </Text>
      </TouchableOpacity>
    </View>
  );
}

function InfoRow({
  label,
  value,
  note,
}: {
  label: string;
  value: string;
  note?: string;
}) {
  return (
    <View style={styles.infoRow}>
      <Text style={styles.infoLabel}>{label}</Text>
      <Text style={styles.infoValue}>{value}</Text>
      {note ? <Text style={styles.infoNote}>{note}</Text> : null}
    </View>
  );
}

const styles = StyleSheet.create({
  card: {
    backgroundColor: '#fff',
    borderRadius: 10,
    padding: 14,
    marginBottom: 8,
    elevation: 3,
  },
  title: {fontSize: 13, color: '#888', marginBottom: 10},
  infoGrid: {gap: 4, marginBottom: 10},
  infoRow: {flexDirection: 'row', alignItems: 'center', gap: 6},
  infoLabel: {
    width: 90,
    fontSize: 12,
    color: '#888',
  },
  infoValue: {
    fontSize: 13,
    fontFamily: 'monospace',
    color: '#1A237E',
    fontWeight: '600',
  },
  infoNote: {fontSize: 10, color: '#bbb'},
  statusRow: {flexDirection: 'row', gap: 16, marginBottom: 12},
  dotRow: {flexDirection: 'row', alignItems: 'center', gap: 6},
  dot: {width: 10, height: 10, borderRadius: 5},
  dotLabel: {fontSize: 12, color: '#555'},
  btn: {
    borderRadius: 8,
    padding: 13,
    alignItems: 'center',
  },
  btnText: {color: '#fff', fontWeight: 'bold', fontSize: 15},
});
