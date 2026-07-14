/*
 * thd.c —— THD 测量业务层
 *
 * 数据流：
 *   ADC DMA 每采满半帧（N=2048）触发 on_adc_frame（ISR 上下文）
 *     ↓ 去 DC + 加 Hann 窗后写入 s_frame，置 s_frame_ready
 *   thd_task 检测到 ready 后：
 *     1) 两级 Goertzel 扫描找 f0
 *     2) 算 H1..H5 复数 Goertzel → 幅度 + 相位
 *     3) EMA 平滑（f0 / 幅度 / THD）
 *     4) auto-Fs：根据 f0 选合适的采样率档位，必要时切换
 *   s_result 保留 EMA 后的结果，供 ui / esp 读取
 *
 * 三大关键改进：
 *   - Hann 窗：泄漏 -13 dB → -32 dB
 *   - N = 2048：观测窗翻倍，Goertzel 分辨率翻倍
 *   - auto-Fs：根据 f0 自动挑档，保证每帧至少 20+ 个信号周期
 */

#include "module/thd.h"
#include "module/goertzel.h"
#include "core/scheduler.h"
#include "bsp/adc.h"
#include "bsp/uart.h"
#include "stm32f1xx_hal.h"
#include <string.h>
#include <math.h>

/* 打开这个宏，thd_task 每 N 帧向 UART 打印一行诊断信息，方便查 auto-Fs / 泄漏问题 */
#define THD_DEBUG_LOG        1
#define THD_DEBUG_EVERY_N    10

/* 调试开关：设 0 = 旁路 Hann 窗（等效于矩形窗，跟老代码一样）
 *          设 1 = 加 Hann 窗（默认，压泄漏）
 * 用来定位"加 Hann 后 f0 就错"的问题。*/
#define THD_USE_HANN_WINDOW  1

/* Auto-Fs 保护：启动后先给这么久让 ADC / EMA 稳定，之后才允许切档 */
#define AUTO_FS_WARMUP_MS    2000U

/* Auto-Fs 保护：H1 幅度小于这个门槛说明没检测到有效信号，不切档
 * （Q15 加窗后的相对幅度；量级取决于信号大小，30~50 是保守值）*/
#define AUTO_FS_MIN_H1       50.0f

/* ============================ 内部状态 ============================ */

static thd_result_t s_result;                                       /* 供外部读取 */
static uint32_t     s_channel = THD_DEFAULT_CHANNEL;
static uint32_t     s_fs_hz   = THD_DEFAULT_FS_HZ;

/* ADC DMA 目标：双帧连成一段 buf，DMA 半满/满时分别喂两半 */
static uint16_t s_dma_buf[THD_N_POINTS * 2];

/* 工作帧：ISR 里把最新一半拷贝到这里并转成 int16，供 task 处理 */
static volatile int16_t s_frame[THD_N_POINTS];
static volatile bool    s_frame_ready = false;

/* Hann 窗系数（Q15 定点，值域 0..32767）。
 * 目的：把矩形窗的旁瓣从 -13 dB 压到 -32 dB，即使每帧只有 1~4 个信号周期
 *      泄漏也会大幅降低，Goertzel 单频精度显著提升。
 * 副作用：主瓣变宽约 2 倍；幅度均匀衰减 √(3/8)≈0.612，THD 是比值不受影响。*/
static int16_t s_window_q15[THD_N_POINTS];

/* ============================ 扫描配置（动态）============================ */

/* 扫描步长/范围随 Fs 变。Goertzel 有效带宽 ≈ Fs/N，加 Hann 窗后主瓣宽 ≈ 2·Fs/N。
 * 粗步长取 2·Fs/N（保证任何真峰都能被一个粗 bin 命中），
 * 粗起点也 = 粗步长（能测到的最低 f0），
 * 细步长 = 粗步长/10（分辨率一档提升），
 * 细范围 = 粗步长（覆盖 ±1 粗 bin）。*/
static float s_coarse_start_hz = 1000.0f;
static float s_coarse_step_hz  = 1000.0f;
static float s_fine_range_hz   = 1000.0f;
static float s_fine_step_hz    =  100.0f;

static void update_scan_params(uint32_t fs_hz)
{
    float bw = 2.0f * (float)fs_hz / (float)THD_N_POINTS;   /* Hann 主瓣宽 */
    if (bw < 10.0f) bw = 10.0f;
    s_coarse_step_hz  = bw;
    s_coarse_start_hz = bw;
    s_fine_range_hz   = bw * 0.5f;    /* 粗定位精度是 ±bw/2，细扫覆盖这个就够 */
    s_fine_step_hz    = bw * 0.1f;
    if (s_fine_step_hz < 1.0f) s_fine_step_hz = 1.0f;
}

/* ============================ Auto-Fs 档位表 ============================ */

/* 从高到低依次是允许使用的采样率。
 * 硬件约束：STM32F103 ADC 极限 ~1 MHz；低端别太低，太低粗扫点不够。
 * 选档规则：最小的 fs 满足 fs ≥ 50·f0，找不到就用第一档。*/
static const uint32_t FS_LEVELS[] = {
    1000000, 500000, 250000, 100000, 50000, 25000
};
#define FS_LEVELS_COUNT (sizeof(FS_LEVELS) / sizeof(FS_LEVELS[0]))

static uint32_t select_fs_for_f0(float f0_hz)
{
    if (f0_hz <= 0.0f) return FS_LEVELS[0];
    float target = 50.0f * f0_hz;
    /* 从最小档开始找满足条件的 */
    for (int i = (int)FS_LEVELS_COUNT - 1; i >= 0; --i) {
        if ((float)FS_LEVELS[i] >= target) return FS_LEVELS[i];
    }
    return FS_LEVELS[0];   /* f0 太高，用最高 Fs */
}

/* ============================ EMA 平滑状态 ============================ */

/* 显示层平滑：EMA 一阶低通，α=0.30，约等于 3~4 帧半衰期，7 帧收敛到 90%。
 * 早期版本用 0.15（更平滑但更慢），为满足"10s 内出结果"提到 0.30。
 * 相位不用标量 EMA（跨 ±π 边界会跳），用复数分量 EMA 后再 atan2。*/
#define EMA_ALPHA        0.30f
#define EMA_ALPHA_INV    (1.0f - EMA_ALPHA)

static float s_ema_f0        = 0.0f;
static float s_ema_thd       = 0.0f;
static float s_ema_h[THD_MAX_HARMONIC];
static float s_ema_re[THD_MAX_HARMONIC];
static float s_ema_im[THD_MAX_HARMONIC];
static bool  s_ema_primed = false;              /* 第一次采样时用测量值直接填 */

/* Fs 切换时重置 EMA（新 Fs 下的 Goertzel 幅度会突变） */
static void ema_reset(void)
{
    s_ema_primed = false;
}

/* Auto-Fs 切换的节流：连续 N 帧不改 Fs，避免频繁停/开 ADC。
 * 从 5 缩短到 2 以加快最终 Fs 收敛（原本 3 档收敛 15 帧，现在 6 帧）。*/
#define AUTO_FS_MIN_FRAMES_BETWEEN_CHANGES  2
static uint32_t s_frames_since_change = 0;
static uint32_t s_start_tick_ms       = 0;

/* ============================ ISR 回调 ============================ */

/* 由 bsp/adc.c 在 DMA 半满 / 满中断里调用，n = THD_N_POINTS */
static void on_adc_frame(const uint16_t *raw, uint16_t n)
{
    if (n != THD_N_POINTS) return;
    if (s_frame_ready) return;     /* 上一帧还没处理，丢弃本帧避免竞态 */

    /* 12-bit ADC 无符号 (0..4095) → int16 有符号并去直流。
     * DC 只影响 0Hz bin，我们从 50Hz 起扫描不受影响，但去掉后计算更干净。 */
    int32_t sum = 0;
    for (uint16_t i = 0; i < n; ++i) sum += raw[i];
    int16_t dc = (int16_t)(sum / n);

    /* 去直流 + 加 Hann 窗（Q15 定点乘法：> 15 即除以 32768）。
     * centered 范围 ±4095，w_q15 范围 0..32767，乘积 ±134M 装得下 int32。
     * 右移 15 位回到 int16 范围。 */
    for (uint16_t i = 0; i < n; ++i) {
        int32_t centered = (int32_t)raw[i] - dc;
#if THD_USE_HANN_WINDOW
        s_frame[i] = (int16_t)((centered * (int32_t)s_window_q15[i]) >> 15);
#else
        s_frame[i] = (int16_t)centered;   /* 矩形窗（旁路 Hann） */
#endif
    }
    s_frame_ready = true;
}

/* ============================ 基频寻峰 ============================ */

/* 用三点抛物线插值精调峰值位置。
 * 输入：mag[c-1], mag[c], mag[c+1] 三个相邻幅度值；step_hz 是相邻点频率间隔。
 * 返回：相对于 mag[c] 中心的偏移（Hz）。
 * 原理：将三点拟合成抛物线 y = a(x-x0)² + b(x-x0) + c，求极值点。 */
static float parabolic_offset_hz(float y_prev, float y_center, float y_next,
                                 float step_hz)
{
    float denom = (y_prev - 2.0f * y_center + y_next);
    if (fabsf(denom) < 1e-6f) return 0.0f;
    /* 极值点相对中心的归一化偏移 (∈ [-0.5, 0.5])，再乘步长换算成 Hz */
    float delta = 0.5f * (y_prev - y_next) / denom;
    if (delta >  0.5f) delta =  0.5f;
    if (delta < -0.5f) delta = -0.5f;
    return delta * step_hz;
}

/* 在 [start, stop] 范围内以 step 步长做 Goertzel 扫描（幅度平方避免开根），
 * 返回峰值下标（相对 start 的整数索引）和峰值前/中/后 3 个幅度值以便插值。 */
static uint32_t goertzel_sweep_argmax(const int16_t *samples, uint32_t n,
                                      uint32_t fs_hz,
                                      float start_hz, float step_hz,
                                      uint32_t n_steps,
                                      float *out_prev, float *out_peak,
                                      float *out_next)
{
    uint32_t peak_idx = 0;
    float    peak_val = -1.0f;
    float    prev_val = 0.0f, next_val = 0.0f;

    /* 保留上一格的幅度，方便 3 点抛物线用 */
    float last_mag2 = 0.0f;

    for (uint32_t i = 0; i < n_steps; ++i) {
        float f = start_hz + (float)i * step_hz;
        if (f >= (float)fs_hz * 0.5f) break;      /* Nyquist 保护 */
        float m2 = goertzel_magnitude2(samples, n, fs_hz, f);

        if (m2 > peak_val) {
            peak_val = m2;
            peak_idx = i;
            prev_val = last_mag2;   /* 保存上一格作为 peak 的 prev */
            next_val = 0.0f;        /* 新峰重置 next，等下一格填 */
        }
        /* 若当前是 peak+1，此时的 m2 就是 next */
        if (i == peak_idx + 1) {
            next_val = m2;
        }
        last_mag2 = m2;
    }

    if (out_prev) *out_prev = prev_val;
    if (out_peak) *out_peak = peak_val;
    if (out_next) *out_next = next_val;
    return peak_idx;
}

/* 两级扫描：粗扫 → 细扫 → 抛物线，返回估计的 f0 */
static float estimate_f0(const int16_t *samples, uint32_t n, uint32_t fs_hz)
{
    /* -------- 粗扫 -------- */
    /* 上限：Fs/10 保证 5 次谐波不越 Nyquist（还留一点抗混叠余量）。*/
    float coarse_stop = (float)fs_hz / 10.0f;
    if (coarse_stop <= s_coarse_start_hz)
        coarse_stop = s_coarse_start_hz + 10.0f * s_coarse_step_hz;
    uint32_t coarse_n = (uint32_t)((coarse_stop - s_coarse_start_hz) / s_coarse_step_hz);
    if (coarse_n < 3) coarse_n = 3;

    float cp = 0, cc = 0, cn = 0;
    uint32_t ci = goertzel_sweep_argmax(samples, n, fs_hz,
                                        s_coarse_start_hz, s_coarse_step_hz,
                                        coarse_n, &cp, &cc, &cn);
    float f_coarse = s_coarse_start_hz + (float)ci * s_coarse_step_hz;

    /* -------- 细扫（在 f_coarse ± fine_range 里以 fine_step 步长扫）-------- */
    float fine_start = f_coarse - s_fine_range_hz;
    if (fine_start < s_coarse_start_hz) fine_start = s_coarse_start_hz;
    uint32_t fine_n = (uint32_t)((2.0f * s_fine_range_hz) / s_fine_step_hz) + 1;

    float fp = 0, fc = 0, fn = 0;
    uint32_t fi = goertzel_sweep_argmax(samples, n, fs_hz,
                                        fine_start, s_fine_step_hz,
                                        fine_n, &fp, &fc, &fn);
    float f_fine = fine_start + (float)fi * s_fine_step_hz;

    /* -------- 抛物线插值精调 -------- */
    /* 如果 fp / fn 拿不到（在边界），插值退化为 0，直接返回 f_fine */
    float delta = parabolic_offset_hz(fp, fc, fn, s_fine_step_hz);
    return f_fine + delta;
}

/* ============================ 谐波与 THD ============================ */

/* 算 H1..H5 的复数 Goertzel，保存 re/im 供上层做相位 EMA。
 * 同时按幅度算 THD_percent 存到 out。*/
static void compute_harmonics_and_thd(const int16_t *samples, uint32_t n,
                                      uint32_t fs_hz, float f0,
                                      thd_result_t *out,
                                      float *out_re, float *out_im)
{
    float nyq = (float)fs_hz * 0.5f;

    for (int k = 1; k <= THD_MAX_HARMONIC; ++k) {
        float f = f0 * (float)k;
        if (f >= nyq || f0 < 1.0f) {
            out->harmonic[k - 1]       = 0.0f;
            out->harmonic_phase[k - 1] = 0.0f;
            out_re[k - 1] = 0.0f;
            out_im[k - 1] = 0.0f;
        } else {
            goertzel_complex_t c = goertzel_complex(samples, n, fs_hz, f);
            out_re[k - 1] = c.re;
            out_im[k - 1] = c.im;
            out->harmonic[k - 1]       = sqrtf(c.re * c.re + c.im * c.im);
            out->harmonic_phase[k - 1] = atan2f(c.im, c.re);
        }
    }

    /* THD = sqrt(H2²+H3²+...+H5²) / H1 × 100 */
    float h1 = out->harmonic[0];
    if (h1 < 1e-3f) {
        out->thd_percent = 0.0f;
        return;
    }
    float sum_sq = 0.0f;
    for (int k = 2; k <= THD_MAX_HARMONIC; ++k) {
        float h = out->harmonic[k - 1];
        sum_sq += h * h;
    }
    out->thd_percent = sqrtf(sum_sq) / h1 * 100.0f;
}

/* ============================ 公开接口 ============================ */

bool thd_is_running(void) { return ADC_IsFFTRunning(); }

void thd_configure(uint32_t channel, uint32_t fs_hz)
{
    if (thd_is_running()) return;   /* 采样中禁止改 */
    s_channel = channel;
    s_fs_hz   = fs_hz ? fs_hz : THD_DEFAULT_FS_HZ;
    update_scan_params(s_fs_hz);
    ema_reset();
}

bool thd_start(void)
{
    if (thd_is_running()) return true;
    memset(&s_result, 0, sizeof(s_result));
    s_result.fs_hz = s_fs_hz;
    s_frame_ready  = false;
    ema_reset();
    s_frames_since_change = 0;
    s_start_tick_ms = HAL_GetTick();

    ADC_SetFFTCallback(on_adc_frame);
    bool ok = ADC_StartFFT(s_channel, s_fs_hz, s_dma_buf,
                           (uint16_t)(THD_N_POINTS * 2));
    if (ok) sched_set_enabled("thd", true);
    return ok;
}

void thd_stop(void)
{
    ADC_StopFFT();
    sched_set_enabled("thd", false);
}

const thd_result_t *thd_get_result(void) { return &s_result; }

/* ============================ Auto-Fs 内部切换 ============================ */

/* 在 task 里调用（不能在 ISR）。停 ADC → 重配 Fs → 重启 ADC。
 * 不动调度器 enabled 状态，避免自己把自己关掉。*/
static bool reconfigure_fs(uint32_t new_fs)
{
    ADC_StopFFT();
    s_fs_hz = new_fs;
    s_result.fs_hz = new_fs;
    update_scan_params(new_fs);
    ema_reset();
    s_frame_ready = false;
    return ADC_StartFFT(s_channel, s_fs_hz, s_dma_buf,
                        (uint16_t)(THD_N_POINTS * 2));
}

/* ============================ 任务函数 ============================ */

void thd_task(void)
{
    if (!s_frame_ready) return;

    const int16_t *samples = (const int16_t *)s_frame;

    /* -------- 1) 估计 f0 -------- */
    float f0 = estimate_f0(samples, THD_N_POINTS, s_fs_hz);

    /* -------- 2) 算原始一帧的谐波和 THD -------- */
    thd_result_t raw;
    memset(&raw, 0, sizeof(raw));
    raw.f0_hz    = f0;
    raw.fs_hz    = s_fs_hz;
    raw.frame_id = s_result.frame_id + 1;

    float re[THD_MAX_HARMONIC], im[THD_MAX_HARMONIC];
    compute_harmonics_and_thd(samples, THD_N_POINTS, s_fs_hz, f0,
                              &raw, re, im);

    /* -------- 3) EMA 平滑 -------- */
    if (!s_ema_primed) {
        /* 首次或 Fs 变更后重置：直接用当前测量值作为起点，避免 0 起步慢慢爬升 */
        s_ema_f0  = raw.f0_hz;
        s_ema_thd = raw.thd_percent;
        for (int k = 0; k < THD_MAX_HARMONIC; ++k) {
            s_ema_h[k]  = raw.harmonic[k];
            s_ema_re[k] = re[k];
            s_ema_im[k] = im[k];
        }
        s_ema_primed = true;
    } else {
        s_ema_f0  = EMA_ALPHA_INV * s_ema_f0  + EMA_ALPHA * raw.f0_hz;
        s_ema_thd = EMA_ALPHA_INV * s_ema_thd + EMA_ALPHA * raw.thd_percent;
        for (int k = 0; k < THD_MAX_HARMONIC; ++k) {
            s_ema_h[k]  = EMA_ALPHA_INV * s_ema_h[k]  + EMA_ALPHA * raw.harmonic[k];
            s_ema_re[k] = EMA_ALPHA_INV * s_ema_re[k] + EMA_ALPHA * re[k];
            s_ema_im[k] = EMA_ALPHA_INV * s_ema_im[k] + EMA_ALPHA * im[k];
        }
    }

    /* -------- 4) 用 EMA 值填 result -------- */
    thd_result_t out;
    out.f0_hz       = s_ema_f0;
    out.thd_percent = s_ema_thd;
    out.fs_hz       = s_fs_hz;
    out.frame_id    = raw.frame_id;
    for (int k = 0; k < THD_MAX_HARMONIC; ++k) {
        out.harmonic[k]       = s_ema_h[k];
        out.harmonic_phase[k] = atan2f(s_ema_im[k], s_ema_re[k]);
    }
    s_result = out;
    s_frame_ready = false;

#if THD_DEBUG_LOG
    if ((raw.frame_id % THD_DEBUG_EVERY_N) == 0) {
        UART_Printf("[thd] fid=%lu Fs=%lu raw_f0=%.1f ema_f0=%.1f THD=%.2f%% "
                    "H=[%.0f %.0f %.0f %.0f %.0f]\r\n",
                    (unsigned long)raw.frame_id,
                    (unsigned long)s_fs_hz,
                    (double)raw.f0_hz,
                    (double)s_ema_f0,
                    (double)s_ema_thd,
                    (double)s_ema_h[0], (double)s_ema_h[1],
                    (double)s_ema_h[2], (double)s_ema_h[3],
                    (double)s_ema_h[4]);
    }
#endif

    /* -------- 5) Auto-Fs 判定：拿 EMA 后的 f0 更稳，不容易抖档 -------- */
    s_frames_since_change++;

    /* Warmup：启动后先让 ADC / EMA 稳定 5 秒，防止冷启动的垃圾数据把 Fs 带偏 */
    if ((HAL_GetTick() - s_start_tick_ms) < AUTO_FS_WARMUP_MS) return;

    if (s_frames_since_change < AUTO_FS_MIN_FRAMES_BETWEEN_CHANGES) return;

    /* 信号强度检查：H1 幅度太小说明没锁到信号（噪声/无输入），不做切档 */
    if (s_ema_h[0] < AUTO_FS_MIN_H1) return;
    if (s_ema_f0 < 20.0f) return;                 /* 无信号或未收敛 */

    uint32_t target = select_fs_for_f0(s_ema_f0);
    if (target == s_fs_hz) return;

    /* 滞回：只在 2× 及以上差距时才切档，避免边界抖动。
     * FS_LEVELS 相邻档差 ≥ 2×，所以任何 target != current 都满足这条。*/
    float ratio = (float)target / (float)s_fs_hz;
    if (ratio > 0.6f && ratio < 1.7f) return;

#if THD_DEBUG_LOG
    UART_Printf("[thd] Fs switch %lu -> %lu (f0=%.1f H1=%.0f)\r\n",
                (unsigned long)s_fs_hz, (unsigned long)target,
                (double)s_ema_f0, (double)s_ema_h[0]);
#endif

    reconfigure_fs(target);
    s_frames_since_change = 0;
}

/* ============================ 初始化 ============================ */

void thd_init(void)
{
    memset(&s_result, 0, sizeof(s_result));
    s_result.fs_hz = s_fs_hz;

    /* 扫描参数按默认 Fs 初始化，thd_configure() 里会再刷一次 */
    update_scan_params(s_fs_hz);

    /* EMA 数组清零；首帧会用测量值 prime */
    memset(s_ema_h,  0, sizeof(s_ema_h));
    memset(s_ema_re, 0, sizeof(s_ema_re));
    memset(s_ema_im, 0, sizeof(s_ema_im));
    s_ema_primed = false;
    s_frames_since_change = 0;

    /* 预计算 Hann 窗：w[n] = 0.5 * (1 - cos(2π n / (N-1)))
     * 存成 Q15 定点 (int16, 0..32767)，采样处理时一次乘法即可。*/
    const float TWO_PI = 6.28318530717958647692f;
    for (uint32_t i = 0; i < THD_N_POINTS; ++i) {
        float w = 0.5f * (1.0f - cosf(TWO_PI * (float)i
                                            / (float)(THD_N_POINTS - 1)));
        int32_t q = (int32_t)(w * 32767.0f + 0.5f);
        if (q > 32767) q = 32767;
        if (q < 0)     q = 0;
        s_window_q15[i] = (int16_t)q;
    }
}
