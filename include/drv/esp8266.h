#ifndef __ESP8266_H
#define __ESP8266_H

#include "stm32f1xx_hal.h"
#include "bsp/uart.h"
#include <stdbool.h>

/* ===== 用户配置（按实际环境修改） ===== */
#define AP_SSID           "21Athd-calculation"
#define AP_PASS           "12345678"
#define PHONE_IP          "192.168.1.2"       /* 手机连热点后默认 IP */
#define PHONE_PORT        8080

#define ESP_TIMEOUT_SHORT 3000U   /* ms */
#define ESP_TIMEOUT_LONG  8000U   /* ms */

/* ===== 野火指南者板 ESP8266 硬件引脚 ===== */
/* USART3 作为通信口（PB10 TX / PB11 RX） */
#define ESP_UART          huart3

/* CH_PD（使能脚）：PB8 高电平有效，不拉高模块不工作 */
#define ESP_CH_PD_PORT    GPIOB
#define ESP_CH_PD_PIN     GPIO_PIN_8
/* RST（复位脚）：PB9 低电平复位 */
#define ESP_RST_PORT      GPIOB
#define ESP_RST_PIN       GPIO_PIN_9

#define ESP_CH_ENABLE()   HAL_GPIO_WritePin(ESP_CH_PD_PORT, ESP_CH_PD_PIN, GPIO_PIN_SET)
#define ESP_CH_DISABLE()  HAL_GPIO_WritePin(ESP_CH_PD_PORT, ESP_CH_PD_PIN, GPIO_PIN_RESET)
#define ESP_RST_HIGH()    HAL_GPIO_WritePin(ESP_RST_PORT,   ESP_RST_PIN,   GPIO_PIN_SET)
#define ESP_RST_LOW()     HAL_GPIO_WritePin(ESP_RST_PORT,   ESP_RST_PIN,   GPIO_PIN_RESET)

/* ===== 返回码 ===== */
typedef enum {
    ESP_OK          = 0,
    ESP_ERR_AT,
    ESP_ERR_TIMEOUT,
    ESP_ERR_PARAM,
} ESP_Result;

/* ===== 统计信息 ===== */
typedef struct {
    bool     ap_ready;
    uint32_t send_count;
    uint32_t fail_count;
    bool     last_send_ok;
} ESP_Stats;

extern ESP_Stats g_esp_stats;

/* ===== 公开接口 =====
 *
 * ESP_Init:
 *   建热点、进入 TCP 模式。开机调一次，阻塞 3-15 秒。
 *
 * ESP_PostThd:
 *   把 THD 结果 POST 给手机（HTTP /thd）。500ms ~ 1s 周期调用即可。
 *   f0/thd 是当前值，h[5] 是 5 次谐波幅度（未归一化），p[5] 是相位（rad）。
 *   阻塞 ~200 ms 到几秒；出错返回非零。
 *
 * ESP_PostSensor:
 *   老 VTL 项目的接口，保留不动方便以后参考；THD 项目不用调。
 */
ESP_Result ESP_Init(void);
ESP_Result ESP_PostThd(float f0_hz, float thd_percent,
                       const float *h5, const float *p5);
ESP_Result ESP_PostSensor(float voltage, bool led_on, uint32_t light,
                          uint8_t temp,
                          float *out_thr, uint32_t *out_light_thr,
                          uint8_t *out_temp_thr);
void       ESP_UART_IRQHandler(void);

#endif /* __ESP8266_H */
