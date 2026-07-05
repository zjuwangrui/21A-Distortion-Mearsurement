#include "drv/light_sensor.h"

/* 硬件绑定：换引脚改这里 */
#define LIGHT_ADC_CHANNEL   ADC_CHANNEL_10
#define LIGHT_GPIO_PORT     GPIOC
#define LIGHT_GPIO_PIN      GPIO_PIN_0

void LightSensor_Init(void)
{
    GPIO_InitTypeDef g = {0};
    g.Pin  = LIGHT_GPIO_PIN;
    g.Mode = GPIO_MODE_ANALOG;
    HAL_GPIO_Init(LIGHT_GPIO_PORT, &g);
}

uint32_t LightSensor_ReadRaw(void)
{
    return ADC_PollRead(LIGHT_ADC_CHANNEL);
}
