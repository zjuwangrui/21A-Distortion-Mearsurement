/**
 * WaveformSvg.tsx — 输入信号一个基频周期的重建波形
 *
 * 用收到的 5 次谐波幅度 h[] 和相位 p[] 合成时域波形：
 *   y(t) = Σ h[k] · cos(2π·(k+1)·t + p[k] - (k+1)·p[0])
 *
 *   - 以 H1 相位为参考 (减 (k+1)·p[0])，波形自动"触发"到基频零相位，稳定不抖
 *   - t ∈ [0, 1)  对应一个基频周期
 *   - x 轴上取 W_POINTS 个点，用 SVG polyline 连起来
 *
 * 依赖 react-native-svg，需 `npm install react-native-svg`。
 */
import React, {useMemo} from 'react';
import {View, Text, StyleSheet} from 'react-native';
import Svg, {Line, Polyline, Rect} from 'react-native-svg';

type Props = {
  h: number[] | null;   // 5 个幅度
  p: number[] | null;   // 5 个相位（弧度）
  width?: number;
  height?: number;
};

const W_POINTS = 200;      // 采样点数（横向像素数量）

export default function WaveformSvg({h, p, width = 320, height = 140}: Props) {
  const points = useMemo(() => {
    if (!h || !p) return null;
    const peak = h.reduce((a, b) => a + b, 0);
    if (peak < 1e-6) return null;

    const scale = (height / 2 - 4) / peak;
    const phi1  = p[0];
    const cy    = height / 2;

    const pts: string[] = [];
    for (let i = 0; i < W_POINTS; i++) {
      const tn = i / W_POINTS;           // 0..1 = 一个基频周期
      let y = 0;
      for (let k = 0; k < 5; k++) {
        if (h[k] < 1e-6) continue;
        const eff = p[k] - (k + 1) * phi1;
        y += h[k] * Math.cos(2 * Math.PI * (k + 1) * tn + eff);
      }
      const x  = (i / (W_POINTS - 1)) * width;
      const py = cy - y * scale;
      pts.push(`${x.toFixed(1)},${py.toFixed(1)}`);
    }
    return pts.join(' ');
  }, [h, p, width, height]);

  return (
    <View style={styles.card}>
      <Text style={styles.title}>输入信号波形（一个基频周期）</Text>
      <View style={{alignItems: 'center'}}>
        <Svg width={width} height={height}>
          <Rect x={0} y={0} width={width} height={height} fill="#000" />
          {/* 零轴 */}
          <Line x1={0} y1={height / 2} x2={width} y2={height / 2}
                stroke="#333" strokeWidth={1} />
          {/* 波形 */}
          {points && (
            <Polyline
              points={points}
              fill="none"
              stroke="#00E676"
              strokeWidth={1.6}
              strokeLinejoin="round"
            />
          )}
        </Svg>
      </View>
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
});
