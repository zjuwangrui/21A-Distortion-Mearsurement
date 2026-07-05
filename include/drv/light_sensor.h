#ifndef __DRV_LIGHT_SENSOR_H
#define __DRV_LIGHT_SENSOR_H

#include "bsp/adc.h"
#include <stdint.h>

/*
 * 示例驱动：光敏电阻分压 → PC0 → ADC1_IN10
 * 换引脚只需改 .c 里 LIGHT_ADC_CHANNEL / LIGHT_GPIO_PORT / LIGHT_GPIO_PIN
 */
void     LightSensor_Init(void);
uint32_t LightSensor_ReadRaw(void);      /* 0..4095 */

#endif /* __DRV_LIGHT_SENSOR_H */
