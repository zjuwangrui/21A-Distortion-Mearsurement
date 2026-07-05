#include "adc.h"

ADC_HandleTypeDef hadc1;

/* PC1 → ADC1_IN11  电压
   PC0 → ADC1_IN10  光照（模拟）
   软件触发，单次转换，轮询读取 */
void MX_ADC1_Init(void)
{
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_ADC1_CLK_ENABLE();
    __HAL_RCC_ADC_CONFIG(RCC_ADCPCLK2_DIV6);   /* 72/6 = 12 MHz */

    GPIO_InitTypeDef gpio = {0};
    gpio.Pin  = GPIO_PIN_0 | GPIO_PIN_1;        /* PC0(光照) + PC1(电压) */
    gpio.Mode = GPIO_MODE_ANALOG;
    HAL_GPIO_Init(GPIOC, &gpio);

    hadc1.Instance                   = ADC1;
    hadc1.Init.ScanConvMode          = ADC_SCAN_DISABLE;
    hadc1.Init.ContinuousConvMode    = DISABLE;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConv      = ADC_SOFTWARE_START;
    hadc1.Init.DataAlign             = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion       = 1;
    if (HAL_ADC_Init(&hadc1) != HAL_OK)
        while (1);

    HAL_ADCEx_Calibration_Start(&hadc1);
}

/* 切换 ADC 通道并单次轮询读取 */
static uint32_t adc_read_channel(uint32_t channel)
{
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

float ADC_ReadVoltage(void)
{
    return adc_read_channel(ADC_CHANNEL_11) / 4095.0f * 3.3f;
}

uint32_t ADC_ReadLightRaw(void)
{
    return adc_read_channel(ADC_CHANNEL_10);
}
