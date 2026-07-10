/**
 * HarmonicsTable.tsx — 基波 + 5 次谐波归一化幅值表格
 *
 * 输入：h[5]（原始幅度，未归一化），内部除以 h[0] 得 Uk/U1。
 * H1 恒等于 1.0000，是基波占位；H2..H5 是相对基波的比例。
 */
import React from 'react';
import {View, Text, StyleSheet} from 'react-native';

type Props = {
  h: number[] | null;   // 5 个元素或 null（未收数据前）
};

export default function HarmonicsTable({h}: Props) {
  const u1 = h && h[0] > 1e-6 ? h[0] : null;
  const norm = (k: number) => {
    if (!h || u1 === null) return '--';
    return (h[k] / u1).toFixed(4);
  };

  const rows = [
    {name: 'H1 (基波)', val: norm(0)},
    {name: 'H2',        val: norm(1)},
    {name: 'H3',        val: norm(2)},
    {name: 'H4',        val: norm(3)},
    {name: 'H5',        val: norm(4)},
  ];

  return (
    <View style={styles.card}>
      <Text style={styles.title}>基波与谐波归一化幅值 (Uk / U1)</Text>
      <View style={styles.headerRow}>
        <Text style={[styles.headerCell, {flex: 2}]}>Harmonic</Text>
        <Text style={[styles.headerCell, {flex: 1, textAlign: 'right'}]}>Uk / U1</Text>
      </View>
      {rows.map((r, i) => (
        <View key={i} style={styles.row}>
          <Text style={[styles.cell, {flex: 2}]}>{r.name}</Text>
          <Text style={[styles.val,  {flex: 1, textAlign: 'right'}]}>{r.val}</Text>
        </View>
      ))}
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
  title: {fontSize: 13, color: '#0D47A1', fontWeight: 'bold', marginBottom: 8},
  headerRow: {
    flexDirection: 'row',
    paddingVertical: 5,
    borderBottomWidth: 1,
    borderBottomColor: '#ddd',
  },
  headerCell: {fontSize: 12, color: '#888'},
  row: {
    flexDirection: 'row',
    paddingVertical: 8,
    borderBottomWidth: 1,
    borderBottomColor: '#f0f0f0',
  },
  cell: {fontSize: 14, color: '#333'},
  val: {
    fontSize: 14,
    color: '#00C853',
    fontFamily: 'monospace',
    fontVariant: ['tabular-nums'],
  },
});
