#include "module/ui.h"
#include "module/thd.h"
#include "drv/esp8266.h"
#include "bsp/lcd.h"
#include "stm32f1xx_hal.h"
#include <string.h>
#include <math.h>

#define UI_MAX_PAGES  4

static const ui_page_t *s_pages[UI_MAX_PAGES];
static uint8_t          s_page_count = 0;
static const ui_page_t *s_current    = 0;
static bool             s_dirty      = true;   /* 需要 enter() 一次 */

/* ============================================================
 *  Page 1: 欢迎页
 * ============================================================ */

static void welcome_enter(void)
{
    LCD_Clear(BLACK);
    LCD_DrawTextC( 40,  30, YELLOW, BLACK, "STM32 is working normally!");
    LCD_DrawTextC( 40,  55, WHITE,  BLACK, "----------------------------");
}

static void welcome_tick(void)
{
    uint32_t s = HAL_GetTick() / 1000U;
    LCD_DrawTextf(40, 200, WHITE, BLACK, "uptime: %lu s   ", (unsigned long)s);
}

static const ui_page_t s_welcome = {
    .name  = "welcome",
    .enter = welcome_enter,
    .tick  = welcome_tick,
};

/* ============================================================
 *  Page 2: THD 显示
 *
 *  布局（320×240 横屏）：
 *   y=  0   Title "THD Meter (Goertzel + Reconstruction)"     16 px
 *   y= 20   waveform 320×50，画  1 个基频周期                    50 px
 *   y= 72   f0                                                   16 px
 *   y= 92   THD                                                  16 px
 *   y=112   "Normalized (Uk/U1):"                                16 px
 *   y=130   H1  1.0000                                           16 px
 *   y=148   H2  0.0324
 *   y=166   H3  0.0152
 *   y=184   H4  0.0056
 *   y=202   H5  0.0031
 *   y=222   Frame # / Fs / RUN|STOP                              16 px
 * ============================================================ */

/* --- 波形显示参数 --- */
#define WAVE_X                0
#define WAVE_Y                20
#define WAVE_W              320       /* 全屏宽 */
#define WAVE_H               50
#define WAVE_CENTER_Y       (WAVE_Y + WAVE_H / 2)
#define WAVE_PIX_PER_PERIOD  320      /* 屏上显示 1 个周期 (题目要求) */
#define WAVE_HALF_H         (WAVE_H / 2 - 2)     /* 波形垂直半幅像素 */

/* --- f0 / THD 独立行 --- */
#define F0_Y                72
#define THD_Y               92

/* --- 归一化幅值显示参数 --- */
#define NORM_HEADER_Y  112          /* "Normalized (Uk/U1):" 标题 */
#define NORM_H1_Y      130          /* 第一行 H1 */
#define NORM_ROW_DY     18          /* 行间距 */
#define NORM_LABEL_X     4          /* "Hk" 起始列 */
#define NORM_VALUE_X    40          /* 数值起始列 */

#define STATUS_Y       222

/* 上一次绘制波形对应的 frame_id；相同则跳过重画避免闪烁 */
static uint32_t s_last_wave_frame = 0xFFFFFFFFU;

/* ------------------------------------------------------------
 *  波形绘制：读取 H[k] 幅度和相位，用 y = ΣA_k cos(2πk·t + φ_k)
 *  在 320 列上算 320 个点然后连线。
 *
 *  技巧：
 *   1) 时间轴用"周期数"归一：t_norm = x / WAVE_PIX_PER_PERIOD
 *      则 cos(2πk·f0·t + φ) = cos(2π·k·t_norm + φ)，f0 自动消掉。
 *   2) 用 H1 的相位 φ_1 做时间参考，画出来的波形相对稳定
 *      （不会因为帧起点漂移而抖动）：
 *          effective φ_k = φ_k - k·φ_1
 *   3) 归一化：估计峰峰值 ≈ Σ|A_k|，把波形缩放到 ±WAVE_HALF_H 像素内。
 * ------------------------------------------------------------ */
static void wave_redraw(const thd_result_t *r)
{
    /* 先清空波形区域 */
    LCD_FillRect(WAVE_X, WAVE_Y, WAVE_W, WAVE_H, BLACK);

    /* 中线（浅蓝，作为零轴参考） */
    LCD_FillRect(WAVE_X, WAVE_CENTER_Y, WAVE_W, 1, BLUE);

    /* 峰峰估计：所有谐波幅度之和（放大保守，避免溢出） */
    float peak_est = 0.0f;
    for (int k = 0; k < THD_MAX_HARMONIC; ++k) peak_est += r->harmonic[k];
    if (peak_est < 1e-3f) return;                /* 无信号，只留中线 */

    /* 相对于 H1 相位的相移，让波形显示"触发"到基频零相位 */
    float phi1 = r->harmonic_phase[0];
    float eff_phi[THD_MAX_HARMONIC];
    for (int k = 0; k < THD_MAX_HARMONIC; ++k)
        eff_phi[k] = r->harmonic_phase[k] - (float)(k + 1) * phi1;

    /* 每像素上的角度增量：对第 k+1 次谐波
     *   Δω_k = 2π · (k+1) / WAVE_PIX_PER_PERIOD */
    const float TWO_PI = 6.28318530717958647692f;
    float scale_y = (float)WAVE_HALF_H / peak_est;

    /* 用绿色连线 */
    LCD_SetColors(GREEN, BLACK);

    int prev_y = -1;
    for (int x = 0; x < WAVE_W; ++x) {
        float t_norm = (float)x / (float)WAVE_PIX_PER_PERIOD;

        /* y(x) = Σ A_k · cos(2π·(k+1)·t_norm + eff_phi_k) */
        float y = 0.0f;
        for (int k = 0; k < THD_MAX_HARMONIC; ++k) {
            float A = r->harmonic[k];
            if (A < 1e-3f) continue;
            float ang = TWO_PI * (float)(k + 1) * t_norm + eff_phi[k];
            y += A * cosf(ang);
        }

        int y_pix = WAVE_CENTER_Y - (int)(y * scale_y);
        /* 硬裁剪，避免画出区域外 */
        if (y_pix < WAVE_Y)             y_pix = WAVE_Y;
        if (y_pix > WAVE_Y + WAVE_H - 1) y_pix = WAVE_Y + WAVE_H - 1;

        /* 相邻两点连线（更像示波器）；第一列没前一点，只画点 */
        if (prev_y < 0) {
            ILI9341_SetPointPixel(x, y_pix);
        } else {
            ILI9341_DrawLine(x - 1, prev_y, x, y_pix);
        }
        prev_y = y_pix;
    }
}

static void thd_enter(void)
{
    LCD_Clear(BLACK);
    LCD_DrawTextC(   4,   0, YELLOW, BLACK, "THD Meter (Goertzel + Reconstruction)");

    /* 波形区域先画一条中线占位 */
    LCD_FillRect(WAVE_X, WAVE_CENTER_Y, WAVE_W, 1, BLUE);

    /* 归一化幅值区标题 + 5 行标签（Uk/U1） */
    LCD_DrawTextC(NORM_LABEL_X, NORM_HEADER_Y, YELLOW, BLACK,
                  "Normalized  Uk/U1 :");
    for (int k = 0; k < THD_MAX_HARMONIC; ++k) {
        LCD_DrawTextf(NORM_LABEL_X, NORM_H1_Y + k * NORM_ROW_DY, WHITE, BLACK,
                      "H%d", k + 1);
    }

    s_last_wave_frame = 0xFFFFFFFFU;    /* 强制首帧重画波形 */
}

static void thd_tick(void)
{
    const thd_result_t *r = thd_get_result();

    /* --- 波形（1 个基频周期）：只在有新帧时重画 --- */
    if (r->frame_id != s_last_wave_frame) {
        s_last_wave_frame = r->frame_id;
        wave_redraw(r);
    }

    /* --- f0 / THD 各占一行，字符宽松，避免挤 --- */
    LCD_DrawTextf(  4, F0_Y,  GREEN, BLACK,
                   "f0 :  %8.2f  kHz    ", (double)r->f0_hz / 1000.0);
    LCD_DrawTextf(  4, THD_Y, r->thd_percent > 5.0f ? RED : GREEN, BLACK,
                   "THD:  %8.3f  %%     ", (double)r->thd_percent);

    /* --- 归一化谐波幅值：Uk/U1 ---
     * H1 = 1.0000 恒定；H2..H5 显示 Um_k / Um_1 (4 位小数)。
     * H1 极小 (无信号) 时全显 0，避免除零抖动。 */
    float u1 = r->harmonic[0];
    bool has_signal = (u1 >= 1e-3f);

    for (int k = 0; k < THD_MAX_HARMONIC; ++k) {
        float norm = has_signal ? (r->harmonic[k] / u1) : 0.0f;
        LCD_DrawTextf(NORM_VALUE_X, NORM_H1_Y + k * NORM_ROW_DY,
                      k == 0 ? GREEN : WHITE, BLACK,
                      "%.4f  ", (double)norm);
    }

    /* --- 状态栏（左侧：帧号+采样率+RUN；右侧：ESP 状态）--- */
    LCD_DrawTextf(4, STATUS_Y, WHITE, BLACK,
                  "#%lu  Fs=%lu  %s   ",
                  (unsigned long)r->frame_id,
                  (unsigned long)r->fs_hz,
                  thd_is_running() ? "RUN " : "STOP");

    /* ESP 状态：直接打字，读出来就知道 OK 还是 FAIL */
    LCD_DrawTextC(238, STATUS_Y, WHITE, BLACK,
                  g_esp_stats.ap_ready ? " ESP OK  " : " ESP FAIL");
}

static const ui_page_t s_thd_page = {
    .name  = "thd",
    .enter = thd_enter,
    .tick  = thd_tick,
};

/* ============================================================
 *  Page registration & switching
 * ============================================================ */

void ui_register_page(const ui_page_t *p)
{
    if (!p || s_page_count >= UI_MAX_PAGES) return;
    s_pages[s_page_count++] = p;
}

bool ui_switch_to(const char *name)
{
    for (uint8_t i = 0; i < s_page_count; ++i) {
        if (strcmp(s_pages[i]->name, name) == 0) {
            s_current = s_pages[i];
            s_dirty   = true;
            return true;
        }
    }
    return false;
}

/* ============================================================
 *  Init / Task
 * ============================================================ */

void ui_init(void)
{
    LCD_Init();
    ui_register_page(&s_welcome);
    ui_register_page(&s_thd_page);
    ui_switch_to("welcome");
}

void ui_task(void)
{
    if (!s_current) return;
    if (s_dirty) {
        if (s_current->enter) s_current->enter();
        s_dirty = false;
    }
    if (s_current->tick) s_current->tick();
}
