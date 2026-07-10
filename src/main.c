/**
 * main.c — STM32 App Framework 骨架 (THD 测量项目)
 *
 * 数据流：
 *   ADC → thd (Goertzel) → thd_result_t
 *                             │
 *                    ┌────────┼─────────┐
 *                    ▼        ▼         ▼
 *                   LCD      esp        (其它)
 *                (ui_task) (esp_task)
 *
 *   esp_task 每 1s 一次，用 AT 指令把结果 HTTP POST 给手机 (AP 客户端)。
 *   手机 App (Expo/RN) 监听 :8080，收到后画卡片+波形+谐波表。
 *
 * 想 debug 打印：include "bsp/uart.h" 后 UART_Printf(...) 就行。
 */

#include "stm32f1xx_hal.h"

#include "bsp/gpio.h"
#include "bsp/led.h"
#include "bsp/uart.h"
#include "bsp/adc.h"

#include "core/scheduler.h"
#include "module/ui.h"
#include "module/thd.h"
#include "drv/ad9910.h"
#include "drv/esp8266.h"

/* ===== 系统时钟：HSE 8MHz × PLL×9 = 72MHz，HSE 失败回退 HSI ===== */
static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef osc = {0};
    RCC_ClkInitTypeDef clk = {0};

    osc.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    osc.HSEState       = RCC_HSE_ON;
    osc.PLL.PLLState   = RCC_PLL_ON;
    osc.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
    osc.PLL.PLLMUL     = RCC_PLL_MUL9;
    if (HAL_RCC_OscConfig(&osc) != HAL_OK) {
        osc.OscillatorType      = RCC_OSCILLATORTYPE_HSI;
        osc.HSIState            = RCC_HSI_ON;
        osc.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
        osc.PLL.PLLSource       = RCC_PLLSOURCE_HSI_DIV2;
        osc.PLL.PLLMUL          = RCC_PLL_MUL16;
        if (HAL_RCC_OscConfig(&osc) != HAL_OK) while (1);
    }

    clk.ClockType      = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK |
                         RCC_CLOCKTYPE_PCLK1  | RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    clk.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV2;
    clk.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_2) != HAL_OK) while (1);
}



/* ===== ESP 上报任务 =====
 * 读最新一帧 THD 结果，通过 AT 命令 HTTP POST 给手机。
 *
 * 注意：ESP_PostThd 是阻塞式的（一次 200 ms ~ 1 s），会占用调度器；
 * 为了不拖垮 UI，把周期设成 1000 ms，同时用 frame_id 去重避免重发。
 *
 * 若 ESP 未初始化成功（s_esp_ready = false），本任务空转 return。 */
static bool     s_esp_ready = false;
static uint32_t s_last_reported_frame = 0xFFFFFFFFU;

static void esp_task(void)
{
    if (!s_esp_ready) return;

    const thd_result_t *r = thd_get_result();
    if (r->frame_id == s_last_reported_frame) return;
    s_last_reported_frame = r->frame_id;

    ESP_PostThd(r->f0_hz, r->thd_percent,
                r->harmonic, r->harmonic_phase);
    /* 失败也不重试 —— 下一轮任务再试即可 */
}

/* ===== 任务表 ===== */
static sched_task_t t_ui        = { .run = ui_task,        .period_ms = 100,  .name = "ui"    };
static sched_task_t t_thd       = { .run = thd_task,       .period_ms = 0,    .name = "thd"   };
static sched_task_t t_esp       = { .run = esp_task,       .period_ms = 1000, .name = "esp"   };

int main(void)
{
    /* ---- 内核 ---- */
    HAL_Init();
    SystemClock_Config();

    /* ---- BSP ---- */
    MX_GPIO_Init();
    MX_USART1_UART_Init();         /* 调试打印口 */
    MX_ADC1_Init();

    /* ---- 模块初始化 ---- */
    ui_init();
    thd_init();

    /* ---- 应用配置 ---- */
    thd_configure(ADC_CHANNEL_11, 1000000);  /* PC1，Fs=1MHz */
    thd_start();
    ui_switch_to("thd");

    /* ---- ESP8266 上报通路（阻塞，最长 ~15s，失败仍继续跑主逻辑） ---- */
    UART_Printf("[main] ESP init...\r\n");
    if (ESP_Init() == ESP_OK) {
        s_esp_ready = true;
        UART_Printf("[main] ESP OK, AP: %s\r\n", AP_SSID);
    } else {
        UART_Printf("[main] ESP FAILED — LCD 仍然工作\r\n");
    }

    /* ---- 调度器 ---- */
    sched_init();
    sched_register(&t_ui);
    sched_register(&t_thd);
    sched_register(&t_esp);

    sched_run_forever();
}
