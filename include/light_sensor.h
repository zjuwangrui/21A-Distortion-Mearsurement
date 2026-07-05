#ifndef __LIGHT_SENSOR_H
#define __LIGHT_SENSOR_H

#include "adc.h"
#include <stdint.h>

/* PC0 → ADC1_IN10，光敏电阻分压输入
 * 引脚已在 MX_ADC1_Init 中配置为 GPIO_MODE_ANALOG */

void     LightSensor_Init(void);
uint32_t LightSensor_ReadRaw(void);   /* 0-4095 */

#endif /* __LIGHT_SENSOR_H */
