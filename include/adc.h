#ifndef __ADC_H
#define __ADC_H

#include "stm32f1xx_hal.h"

extern ADC_HandleTypeDef hadc1;

void     MX_ADC1_Init(void);
float    ADC_ReadVoltage(void);   /* PC1 / IN11 → 电压 V */
uint32_t ADC_ReadLightRaw(void);  /* PC0 / IN10 → 光照 0-4095 */

#endif /* __ADC_H */
