#include "bsp/adc.h"

ADC_HandleTypeDef hadc1;
TIM_HandleTypeDef htim3;
DMA_HandleTypeDef hdma_adc1;

/* ===== FFT 通路状态 ===== */
static adc_fft_cb_t s_fft_cb      = 0;
static uint16_t    *s_fft_buf     = 0;
static uint16_t     s_fft_len     = 0;
static bool         s_fft_running = false;

/* ===== 内部：低速轮询 ADC 通用初始化 ===== */
static void adc1_init_polling(void)
{
    hadc1.Instance                   = ADC1;
    hadc1.Init.ScanConvMode          = ADC_SCAN_DISABLE;
    hadc1.Init.ContinuousConvMode    = DISABLE;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConv      = ADC_SOFTWARE_START;
    hadc1.Init.DataAlign             = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion       = 1;
    if (HAL_ADC_Init(&hadc1) != HAL_OK) while (1);
    HAL_ADCEx_Calibration_Start(&hadc1);
}

void MX_ADC1_Init(void)
{
    __HAL_RCC_ADC1_CLK_ENABLE();
    __HAL_RCC_ADC_CONFIG(RCC_ADCPCLK2_DIV6);   /* 72/6 = 12 MHz */
    adc1_init_polling();
}

/* ===== 低速轮询 ===== */
uint32_t ADC_PollRead(uint32_t channel)
{
    if (s_fft_running) return 0;

    ADC_ChannelConfTypeDef ch = {0};
    ch.Channel      = channel;
    ch.Rank         = ADC_REGULAR_RANK_1;
    ch.SamplingTime = ADC_SAMPLETIME_55CYCLES_5;
    HAL_ADC_ConfigChannel(&hadc1, &ch);

    HAL_ADC_Start(&hadc1);
    if (HAL_ADC_PollForConversion(&hadc1, 10) != HAL_OK) {
        HAL_ADC_Stop(&hadc1);
        return 0;
    }
    uint32_t raw = HAL_ADC_GetValue(&hadc1);
    HAL_ADC_Stop(&hadc1);
    return raw;
}

float ADC_PollReadVoltage(uint32_t channel)
{
    return ADC_PollRead(channel) / 4095.0f * 3.3f;
}

/* ==================== FFT 高速通路 ==================== */

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

    HAL_ADC_DeInit(&hadc1);
    hadc1.Instance                   = ADC1;
    hadc1.Init.ScanConvMode          = ADC_SCAN_DISABLE;
    hadc1.Init.ContinuousConvMode    = DISABLE;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConv      = ADC_EXTERNALTRIGCONV_T3_TRGO;
    hadc1.Init.DataAlign             = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion       = 1;
    if (HAL_ADC_Init(&hadc1) != HAL_OK) return false;
    HAL_ADCEx_Calibration_Start(&hadc1);

    ADC_ChannelConfTypeDef ch = {0};
    ch.Channel      = channel;
    ch.Rank         = ADC_REGULAR_RANK_1;
    ch.SamplingTime = ADC_SAMPLETIME_13CYCLES_5;   /* 快采样，高 Fs 也 OK */
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
    adc1_init_polling();

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
