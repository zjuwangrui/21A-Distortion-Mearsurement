#ifndef __BSP_ADC_H
#define __BSP_ADC_H

#include "stm32f1xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

/* ===== 通用 ADC1 服务 =====
 *
 * 两种模式（互斥，同时只能开一种）：
 *   1) 低速轮询：ADC_PollRead / ADC_PollReadVoltage
 *      任意通道，一次一读，软件触发。
 *   2) FFT 高速采样：ADC_StartFFT / ADC_StopFFT
 *      单通道，TIM3 TRGO 触发，DMA1 循环，Fs 可配（默认 40kHz）。
 *
 * 框架层不假设具体通道分配。业务代码/驱动自己声明用哪个 IN，
 * 并在自己 init 时把对应 GPIO 配成 GPIO_MODE_ANALOG。
 */

extern ADC_HandleTypeDef hadc1;
extern TIM_HandleTypeDef htim3;
extern DMA_HandleTypeDef hdma_adc1;

/* FFT 采样完成一半/满时的回调：由 ADC 模块在中断里调用。
 * raw 指向新到的一段 uint16_t 数据（长度 n）。*/
typedef void (*adc_fft_cb_t)(const uint16_t *raw, uint16_t n);

/* ---- 初始化 ---- */
void MX_ADC1_Init(void);

/* ---- 低速轮询 ----
 * channel: ADC_CHANNEL_0 ... ADC_CHANNEL_17 之一（如 ADC_CHANNEL_10）
 * 返回 12-bit 原始值 (0..4095)。若正处于 FFT 模式则返回 0。
 */
uint32_t ADC_PollRead(uint32_t channel);
float    ADC_PollReadVoltage(uint32_t channel);   /* 已换算成 V，参考 3.3V */

/* ---- FFT 高速采样 ---- */
void ADC_SetFFTCallback(adc_fft_cb_t cb);
bool ADC_StartFFT(uint32_t channel, uint32_t fs_hz,
                  uint16_t *buf, uint16_t buf_len);  /* buf_len 必须偶数 */
void ADC_StopFFT(void);
bool ADC_IsFFTRunning(void);

#endif /* __BSP_ADC_H */
