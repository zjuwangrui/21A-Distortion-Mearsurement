#include "bsp/gpio.h"

/*
 * 框架层不假设具体引脚。各模块（uart / adc / esp8266 / ad9910 / spi …）
 * 在自己 init 里配置自己的 GPIO（含时钟使能 + pin mode）。
 *
 * 本项目实际用到：
 *   GPIOA  — USART1 (PA9/10)，SPI1 (PA5/6/7)，AD9910 CS (PA4)
 *   GPIOB  — USART3 (PB10/11)，ESP CH_PD/RST (PB8/9)，AD9910 RESET/IO_UP (PB1/2)
 *   GPIOC  — ADC 输入 PC1 (在 MX_ADC1_Init 里配)
 *
 * D/E/AFIO 本项目没用到，不使能。
 * 这里的中央使能是"防御性"—— 万一某个模块 init 顺序颠倒也不至于访问未使能的口。
 */
void MX_GPIO_Init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
}
