#include "light_sensor.h"

void LightSensor_Init(void) { /* PC0 已在 MX_ADC1_Init 配置，无需额外初始化 */ }

uint32_t LightSensor_ReadRaw(void)
{
    return ADC_ReadLightRaw();
}
