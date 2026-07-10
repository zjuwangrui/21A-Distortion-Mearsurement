#include "bsp/adc.h"

ADC_HandleTypeDef hadc1;
TIM_HandleTypeDef htim3;
DMA_HandleTypeDef hdma_adc1;

/* ===== FFT 通路状态 ===== */
static adc_fft_cb_t s_fft_cb      = 0;
static uint16_t    *s_fft_buf     = 0;
static uint16_t     s_fft_len     = 0;
static bool         s_fft_running = false;

/* ADC1 基础初始化：时钟 + 分频 + 输入引脚模拟模式。
 * 具体 HAL_ADC_Init 在 ADC_StartFFT 里做（那时才知道触发源和通道）。
 *
 * 本项目输入引脚：PC1 (ADC1_IN11)。换脚时同时改这里的 GPIO + main.c
 * 里 thd_configure(通道号, ...) 两处。 */
void MX_ADC1_Init(void)
{
    __HAL_RCC_ADC1_CLK_ENABLE();
    /* ADCCLK 分频：
     *   /6 = 12 MHz  安全（F103 手册上限 14 MHz），但单 ADC 只到 857 kSps
     *   /4 = 18 MHz  超规 28%，实测大多数 F103/GD32 芯片能跑，可到 1.28 MSps
     * 为达到 THD 采样 1 MHz，选 /4；线性度可能略降，
     * 若测出的 THD 明显偏大，回退到 /6 并把 Fs 降到 500 kHz 复测对比。 */
    __HAL_RCC_ADC_CONFIG(RCC_ADCPCLK2_DIV4);   /* 72/4 = 18 MHz (over-spec) */

    /* ---- 输入引脚：PC1 → ADC1_IN11 ---- */
    __HAL_RCC_GPIOC_CLK_ENABLE();
    GPIO_InitTypeDef g = {0};
    g.Pin  = GPIO_PIN_1;
    g.Mode = GPIO_MODE_ANALOG;
    g.Pull = GPIO_NOPULL;                       /* 模拟输入应关闭上下拉 */
    HAL_GPIO_Init(GPIOC, &g);
}

void ADC_SetFFTCallback(adc_fft_cb_t cb) { s_fft_cb = cb; }
bool ADC_IsFFTRunning(void)              { return s_fft_running; }

/* TIM3 → ADC1 触发：Fs = tim_clk / (ARR+1)
 * APB1=36MHz，预分频≠1 时 timer_clk = 2×APB1 = 72MHz。
 */
static void tim3_configure_start(uint32_t fs_hz)
{
    __HAL_RCC_TIM3_CLK_ENABLE();

    htim3.Instance               = TIM3;
    htim3.Init.Prescaler         = 0;
    htim3.Init.Period            = (72000000UL / fs_hz) - 1;
    htim3.Init.CounterMode       = TIM_COUNTERMODE_UP;
    htim3.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_Base_Init(&htim3) != HAL_OK) while (1);

    TIM_MasterConfigTypeDef mst = {0};
    mst.MasterOutputTrigger = TIM_TRGO_UPDATE;
    mst.MasterSlaveMode     = TIM_MASTERSLAVEMODE_DISABLE;
    HAL_TIMEx_MasterConfigSynchronization(&htim3, &mst);

    HAL_TIM_Base_Start(&htim3);
}

static void tim3_stop(void)
{
    HAL_TIM_Base_Stop(&htim3);
    HAL_TIM_Base_DeInit(&htim3);
    __HAL_RCC_TIM3_CLK_DISABLE();
}

static void dma_link_start(uint16_t *buf, uint16_t len)
{
    __HAL_RCC_DMA1_CLK_ENABLE();

    hdma_adc1.Instance                 = DMA1_Channel1;
    hdma_adc1.Init.Direction           = DMA_PERIPH_TO_MEMORY;
    hdma_adc1.Init.PeriphInc           = DMA_PINC_DISABLE;
    hdma_adc1.Init.MemInc              = DMA_MINC_ENABLE;
    hdma_adc1.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    hdma_adc1.Init.MemDataAlignment    = DMA_MDATAALIGN_HALFWORD;
    hdma_adc1.Init.Mode                = DMA_CIRCULAR;
    hdma_adc1.Init.Priority            = DMA_PRIORITY_HIGH;
    if (HAL_DMA_Init(&hdma_adc1) != HAL_OK) while (1);

    __HAL_LINKDMA(&hadc1, DMA_Handle, hdma_adc1);

    HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);

    HAL_ADC_Start_DMA(&hadc1, (uint32_t *)buf, len);
}

bool ADC_StartFFT(uint32_t channel, uint32_t fs_hz,
                  uint16_t *buf, uint16_t buf_len)
{
    if (s_fft_running || !buf || (buf_len & 1U) || fs_hz == 0) return false;

    hadc1.Instance                   = ADC1;
    hadc1.Init.ScanConvMode          = ADC_SCAN_DISABLE;
    hadc1.Init.ContinuousConvMode    = DISABLE;                     /* 每次触发一次 */
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConv      = ADC_EXTERNALTRIGCONV_T3_TRGO;
    hadc1.Init.DataAlign             = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion       = 1;
    if (HAL_ADC_Init(&hadc1) != HAL_OK) return false;
    HAL_ADCEx_Calibration_Start(&hadc1);

    ADC_ChannelConfTypeDef ch = {0};
    ch.Channel      = channel;
    ch.Rank         = ADC_REGULAR_RANK_1;
    /* 最短采样时间。18MHz ADCCLK 下 1.5+12.5=14 cycles = 0.78μs/次 → 1.28 MSps
     * 前提：输入源阻抗 < ~500Ω。AD9910 经运放缓冲的输出满足这个条件。
     * 若输入接高阻分压器，加运放跟随，否则采样保持电容充不满 → 精度掉。 */
    ch.SamplingTime = ADC_SAMPLETIME_1CYCLE_5;
    HAL_ADC_ConfigChannel(&hadc1, &ch);

    s_fft_buf     = buf;
    s_fft_len     = buf_len;
    s_fft_running = true;

    dma_link_start(buf, buf_len);
    tim3_configure_start(fs_hz);
    return true;
}

void ADC_StopFFT(void)
{
    if (!s_fft_running) return;

    tim3_stop();
    HAL_ADC_Stop_DMA(&hadc1);
    HAL_NVIC_DisableIRQ(DMA1_Channel1_IRQn);
    HAL_DMA_DeInit(&hdma_adc1);
    HAL_ADC_DeInit(&hadc1);

    s_fft_running = false;
    s_fft_buf     = 0;
    s_fft_len     = 0;
}

/* ===== DMA 完成回调（HAL 弱符号覆盖） ===== */
void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef *hadc)
{
    if (hadc->Instance == ADC1 && s_fft_cb && s_fft_buf) {
        s_fft_cb(s_fft_buf, s_fft_len / 2);
    }
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    if (hadc->Instance == ADC1 && s_fft_cb && s_fft_buf) {
        s_fft_cb(s_fft_buf + s_fft_len / 2, s_fft_len / 2);
    }
}
