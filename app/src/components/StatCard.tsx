/**
 * StatCard.tsx — 通用大数字卡片（用于 f0 / THD 两个指标）
 *
 * Props:
 *   label   : 顶部小标签，如 "基频 f0"
 *   value   : 主数字，如 "1000.50"
 *   unit    : 单位，如 "Hz"
 *   accent  : 数字颜色（默认绿色，THD 超阈值时传红色）
 *   subtext : 副信息，如 "最近更新: 10:34:22"
 */
import React from 'react';
import {View, Text, StyleSheet} from 'react-native';

type Props = {
  label: string;
  value: string;
  unit?: string;
  accent?: string;
  subtext?: string;
};

export default function StatCard({label, value, unit, accent = '#00C853', subtext}: Props) {
  return (
    <View style={styles.card}>
      <Text style={styles.label}>{label}</Text>
      <View style={styles.row}>
        <Text style={[styles.value, {color: accent}]}>{value}</Text>
        {unit ? <Text style={styles.unit}>{unit}</Text> : null}
      </View>
      {subtext ? <Text style={styles.sub}>{subtext}</Text> : null}
    </View>
  );
}

const styles = StyleSheet.create({
  card: {
    backgroundColor: '#fff',
    borderRadius: 10,
    padding: 14,
    marginBottom: 10,
    elevation: 3,
  },
  label: {fontSize: 12, color: '#888', marginBottom: 4},
  row: {flexDirection: 'row', alignItems: 'baseline'},
  value: {
    fontSize: 32,
    fontWeight: 'bold',
    fontVariant: ['tabular-nums'],
    letterSpacing: 0.5,
  },
  unit: {fontSize: 16, color: '#666', marginLeft: 6},
  sub: {fontSize: 11, color: '#aaa', marginTop: 4},
});
