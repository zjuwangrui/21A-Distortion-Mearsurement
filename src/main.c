/**
 * main.c — VTL Sensor System (STM32F103VE 野火指南者)
 *
 * 引脚说明（对应CubeMX配置）:
 *   PA0  → ADC1_IN0   被测电压输入（0~3.3V，注意不要超压）
 *   PB10 → USART3_TX  连 ESP8266 RX
 *   PB11 → USART3_RX  连 ESP8266 TX
 *   PB8  → GPIO_OUT   ESP8266 CH_PD（使能，高电平有效）
 *   PB9  → GPIO_OUT   ESP8266 RST（低电平复位）
 *   PB5  → GPIO_OUT   LED_RED（野火指南者板载红色LED，低电平亮）
 *   PB0  → GPIO_OUT   LED_GREEN（可选，高阈值指示）
 *   PA9  → USART1_TX  调试串口（ST-Link，波特率115200）
 *
 * CubeMX必须开启:
 *   ADC1  IN0  独立模式  软件触发
 *   USART1 115200 8N1 Async（调试，可选）
 *   USART2 115200 8N1 Async  + 开启全局中断
 *   GPIOB PB5 PB0 Output_PP
 *   时钟:  内外晶振均可，推荐72MHz
 */

/* USER CODE BEGIN Includes */
#include "esp8266.h"
#include "stm32f1xx_hal.h"
#include "uart.h"
#include "adc.h"
#include "gpio.h"
#include "lcd.h"
#include "light_sensor.h"
#include "dht11.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
/* USER CODE END Includes */

/* ===== LED引脚（野火指南者，低电平亮） ===== */
#define LED_RED_PORT   GPIOB
#define LED_RED_PIN    GPIO_PIN_5
#define LED_GREEN_PORT GPIOB
#define LED_GREEN_PIN  GPIO_PIN_0

#define LED_ON(port, pin)   HAL_GPIO_WritePin(port, pin, GPIO_PIN_RESET)
#define LED_OFF(port, pin)  HAL_GPIO_WritePin(port, pin, GPIO_PIN_SET)

/* ===== 调试打印（通过USART1发到PC，可用串口助手查看） ===== */
#define DBG(fmt, ...) UART_Printf("[%6lu] " fmt "\r\n", HAL_GetTick(), ##__VA_ARGS__)

/* ===== 系统时钟：HSE 8MHz × PLL×9 = 72MHz，HSE 失败则回退 HSI ===== */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef osc = {0};
    RCC_ClkInitTypeDef clk = {0};

    /* 先尝试 HSE 8MHz × PLL×9 = 72MHz */
    osc.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    osc.HSEState       = RCC_HSE_ON;
    osc.PLL.PLLState   = RCC_PLL_ON;
    osc.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
    osc.PLL.PLLMUL     = RCC_PLL_MUL9;
    if (HAL_RCC_OscConfig(&osc) != HAL_OK) {
        /* HSE 起振失败，回退 HSI/2 × PLL×16 = 64MHz */
        osc.OscillatorType          = RCC_OSCILLATORTYPE_HSI;
        osc.HSIState                = RCC_HSI_ON;
        osc.HSICalibrationValue     = RCC_HSICALIBRATION_DEFAULT;
        osc.PLL.PLLSource           = RCC_PLLSOURCE_HSI_DIV2;
        osc.PLL.PLLMUL              = RCC_PLL_MUL16;
        if (HAL_RCC_OscConfig(&osc) != HAL_OK)
            while (1);
    }

    clk.ClockType      = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK |
                         RCC_CLOCKTYPE_PCLK1  | RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    clk.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV2;
    clk.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_2) != HAL_OK)
        while (1);
}

/* ===== 主函数 ===== */
int main(void)
{
    /* --- 初始化 --- */
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_ADC1_Init();
    LightSensor_Init();
    DHT11_Init();
    MX_USART1_UART_Init();
    MX_USART3_UART_Init();
    LCD_DisplayInit();

    /* --- 上电状态：两个LED全灭 --- */
    LED_OFF(LED_RED_PORT,   LED_RED_PIN);
    LED_OFF(LED_GREEN_PORT, LED_GREEN_PIN);

    /* --- 初始化ESP8266（阻塞，最长约15s） --- */
    DBG("ESP8266 initializing...");
    LCD_ShowBoot();
    ESP_Result init_result;
    uint8_t retry = 0;
    do {
        init_result = ESP_Init();
        if (init_result == ESP_OK) break;
        DBG("ESP init failed, retry %d", retry);
        HAL_Delay(2000);
    } while (++retry < 3);

    if (init_result == ESP_OK) {
        DBG("ESP8266 AP ready: SSID=%s", AP_SSID);
        LCD_ShowWiFiStatus(true);
        for (int i = 0; i < 3; i++) {
            LED_ON(LED_GREEN_PORT, LED_GREEN_PIN);
            HAL_Delay(200);
            LED_OFF(LED_GREEN_PORT, LED_GREEN_PIN);
            HAL_Delay(200);
        }
    } else {
        DBG("ESP8266 init FAILED");
        LCD_ShowESPFail();
        for (;;) {
            LED_ON(LED_RED_PORT, LED_RED_PIN);
            HAL_Delay(100);
            LED_OFF(LED_RED_PORT, LED_RED_PIN);
            HAL_Delay(100);
        }
    }

    /* ===== 主循环变量 ===== */
    float    voltage       = 0.0f;
    float    v_threshold   = 1.65f;   /* 电压阈值，手机下发后更新 */
    uint32_t light         = 0;
    uint32_t light_thr     = 2000;    /* 光照阈值（0-4095），手机下发后更新 */
    uint8_t  temp          = 0;       /* 温度°C */
    uint8_t  temp_thr      = 30;      /* 温度阈值°C，手机下发后更新 */
    bool     led_on        = false;
    uint32_t last_send_tick = 0;
    uint32_t last_dht_tick  = 0;
    const uint32_t SEND_INTERVAL_MS = 3000;
    const uint32_t DHT_INTERVAL_MS  = 2000;

    /* ===== 主循环 ===== */
    while (1)
    {
        /* 1. 读取传感器 */
        voltage = ADC_ReadVoltage();
        light   = LightSensor_ReadRaw();

        /* DHT11 每 2s 读一次（协议需要至少 1s 间隔） */
        if (HAL_GetTick() - last_dht_tick >= DHT_INTERVAL_MS) {
            last_dht_tick = HAL_GetTick();
            DHT11_Data_t dht;
            if (DHT11_Read(&dht) == HAL_OK) {
                temp = dht.temp_int;
            }
        }

        /* 2. LED 判断：电压、光照或温度任一超阈值 → 报警 */
        bool new_led = (voltage > v_threshold) || (light > light_thr) || (temp > temp_thr);
        if (new_led != led_on) {
            led_on = new_led;
            if (led_on) {
                LED_ON(LED_RED_PORT, LED_RED_PIN);
                LED_OFF(LED_GREEN_PORT, LED_GREEN_PIN);
            } else {
                LED_OFF(LED_RED_PORT, LED_RED_PIN);
                LED_ON(LED_GREEN_PORT, LED_GREEN_PIN);
            }
        }

        /* 3. 刷新 LCD */
        LCD_UpdateSensor(voltage, v_threshold, light, light_thr,
                         temp, temp_thr);

        /* 4. 定时向手机发送数据，取回阈值 */
        if (HAL_GetTick() - last_send_tick >= SEND_INTERVAL_MS) {
            last_send_tick = HAL_GetTick();

            float    new_v_thr = v_threshold;
            uint32_t new_l_thr = light_thr;
            uint8_t  new_t_thr = temp_thr;
            ESP_Result r = ESP_PostSensor(voltage, led_on, light, temp,
                                          &new_v_thr, &new_l_thr, &new_t_thr);

            if (r == ESP_OK) {
                v_threshold = new_v_thr;
                light_thr   = new_l_thr;
                temp_thr    = new_t_thr;
                DBG("POST OK  V=%.3f L=%lu T=%d LED=%d VTHR=%.3f LTHR=%lu TTHR=%d",
                    voltage, (unsigned long)light, (int)temp, led_on,
                    v_threshold, (unsigned long)light_thr, (int)temp_thr);
            } else {
                DBG("POST FAIL(code=%d) V=%.3f L=%lu T=%d LED=%d",
                    r, voltage, (unsigned long)light, (int)temp, led_on);
            }
        }

        /* 不需要delay，ADC+UART已经消耗了足够时间 */
    }
}

/* ===== UART中断回调（在usart.c或此文件末尾放置） ===== */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART3) {
        ESP_UART_IRQHandler();
    }
}
