/**
 * main.c — STM32 App Framework 骨架
 *
 * 只做：时钟 → BSP 初始化 → 注册核心任务 → 交给调度器。
 *
 * 模块配置通过 C 代码（在下面 main() 里直接调 xxx_configure / xxx_start），
 * 无 CLI。想改参数就改这几行然后重新烧录，比赛也稳定。
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

/* ===== 心跳任务：红灯 500ms 翻转，程序在跑的可视化指示 ===== */
static void heartbeat_task(void) { LED_Toggle(LED_RED); }

/* ===== 任务表（增删模块改这里） ===== */
static sched_task_t t_heartbeat = { .run = heartbeat_task, .period_ms = 500, .name = "heart" };
static sched_task_t t_ui        = { .run = ui_task,        .period_ms = 100, .name = "ui"    };
static sched_task_t t_thd       = { .run = thd_task,       .period_ms = 0,   .name = "thd"   };

int main(void)
{
    /* ---- 内核 ---- */
    HAL_Init();
    SystemClock_Config();

    /* ---- BSP ---- */
    MX_GPIO_Init();
    LED_Init();
    MX_USART1_UART_Init();         /* 打印用；不开 RX IT */
    MX_ADC1_Init();

    /* ---- 模块初始化 ---- */
    ui_init();
    thd_init();
    ad9910_init();                 /* AD9910 上电 + PLL + 默认 1kHz 输出 */

    /* ---- 应用配置 ----
     * 参数都在这里定死，改完就烧。比赛版本改完不再动。 */
    thd_configure(ADC_CHANNEL_1, 400000);  /* THD 测量：PA1，Fs=400kHz */
    thd_start();
    ui_switch_to("thd");

    /* ---- 调度器 ---- */
    sched_init();
    sched_register(&t_heartbeat);
    sched_register(&t_ui);
    sched_register(&t_thd);

    sched_run_forever();           /* 不返回 */
}
