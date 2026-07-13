#ifndef __MODULE_THD_H
#define __MODULE_THD_H

#include <stdint.h>
#include <stdbool.h>

/*
 * ===========================================================================
 *  THD 测量模块（基于 Goertzel + Hann 窗 + Auto-Fs + EMA 平滑）
 * ===========================================================================
 *
 * 输入通路：
 *   ADC1（TIM3 触发）→ DMA1 双缓冲 → on_adc_frame（ISR）
 *   帧内：去 DC + 加 Hann 窗后写入 s_frame
 *
 * 处理流程（thd_task 里）：
 *   1) 两级 Goertzel 扫描找 f0（步长/范围随 Fs 动态调整）
 *   2) 复数 Goertzel 算 H1..H5 幅度和相位
 *   3) EMA 一阶低通平滑（α=0.15，约 6~8 帧收敛）
 *   4) Auto-Fs：根据 f0 从档位表里挑最合适的 Fs（保证 Fs ≥ 50·f0），
 *      至少间隔 5 帧才允许再改，避免抖档
 *   5) THD = √(H2²+H3²+H4²+H5²) / H1 × 100 %
 *
 * 用法：
 *   thd_start();          // 独占 ADC，开始采样（auto-Fs 会自动追频）
 *   ...
 *   const thd_result_t *r = thd_get_result();  // 拿到 EMA 后的稳定值
 *   ...
 *   thd_stop();
 * ===========================================================================
 */

#define THD_N_POINTS         2048U
#define THD_DEFAULT_FS_HZ    1000000U   /* 1 MHz 起步，auto-Fs 会按 f0 自动降档 */
#define THD_DEFAULT_CHANNEL  11U        /* ADC_CHANNEL_11 = PC1 */
#define THD_MAX_HARMONIC     5          /* 算到 5 次谐波 */

typedef struct {
    float    f0_hz;                                /* 检测到的基频 Hz            */
    float    harmonic[THD_MAX_HARMONIC];           /* H[0]=H1 幅度 … H[4]=H5     */
    float    harmonic_phase[THD_MAX_HARMONIC];     /* 相位 (radians, -π..π)     */
    float    thd_percent;                          /* 失真度 %                    */
    uint32_t frame_id;                             /* 每算一帧自增                */
    uint32_t fs_hz;                                /* 当前采样率                  */
} thd_result_t;

void  thd_init(void);
void  thd_task(void);                      /* 注册到调度器 */
bool  thd_start(void);
void  thd_stop (void);
bool  thd_is_running(void);
void  thd_configure(uint32_t channel, uint32_t fs_hz);
const thd_result_t *thd_get_result(void);

#endif /* __MODULE_THD_H */
