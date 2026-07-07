#include "module/ui.h"
#include "module/thd.h"
#include "module/terminal.h"
#include "bsp/lcd.h"
#include "stm32f1xx_hal.h"
#include <string.h>

#define UI_MAX_PAGES  4

static const ui_page_t *s_pages[UI_MAX_PAGES];
static uint8_t          s_page_count = 0;
static const ui_page_t *s_current    = 0;
static bool             s_dirty      = true;   /* 需要 enter() 一次 */

/* ============================================================
 *  Page 1: 欢迎页
 * ============================================================ */

static void welcome_enter(void)
{
    LCD_Clear(BLACK);
    LCD_DrawTextC( 40,  30, YELLOW, BLACK, "STM32 App Framework");
    LCD_DrawTextC( 40,  55, WHITE,  BLACK, "----------------------------");
    LCD_DrawTextC( 40,  80, WHITE,  BLACK, "core   : scheduler + ring");
    LCD_DrawTextC( 40,  98, WHITE,  BLACK, "module : terminal + thd + ui");
    LCD_DrawTextC( 40, 116, WHITE,  BLACK, "bsp    : uart + adc + lcd + led");
    LCD_DrawTextC( 40, 150, GREEN,  BLACK, "Type 'help' on USART1 to start");
}

static void welcome_tick(void)
{
    uint32_t s = HAL_GetTick() / 1000U;
    LCD_DrawTextf(40, 200, WHITE, BLACK, "uptime: %lu s   ", (unsigned long)s);
}

static const ui_page_t s_welcome = {
    .name  = "welcome",
    .enter = welcome_enter,
    .tick  = welcome_tick,
};

/* ============================================================
 *  Page 2: THD 显示
 *   ┌──────────────────────────────────────┐
 *   │ THD Meter                            │  y=4
 *   │ f0: 999.5 Hz     THD: 0.324 %        │  y=30
 *   │ H1 ████████████████████ 1.000        │  y=60
 *   │ H2 █                    0.032        │  y=80
 *   │ H3 ▏                    0.015        │  y=100
 *   │ H4                      0.005        │  y=120
 *   │ H5                      0.003        │  y=140
 *   │                                      │
 *   │ Frame #12345   Fs 40000 Hz  RUN      │  y=200
 *   └──────────────────────────────────────┘
 * ============================================================ */

#define THD_BAR_X       40       /* 条形起始 x  */
#define THD_BAR_MAX_W  180       /* 条形最大宽度 (px) */
#define THD_BAR_H       12       /* 条形高度 */
#define THD_VAL_X      230       /* 数值起始 x */
#define THD_H1_Y        60
#define THD_ROW_DY      20

/* 每个谐波上一次画的条宽，用于只擦"多出来"的部分，减少闪烁 */
static int s_last_bar_w[THD_MAX_HARMONIC] = {0};

static void thd_enter(void)
{
    LCD_Clear(BLACK);
    LCD_DrawTextC(   4,   4, YELLOW, BLACK, "THD Meter (Goertzel)");
    LCD_DrawTextC(   4,  30, WHITE,  BLACK, "f0:");
    LCD_DrawTextC( 150,  30, WHITE,  BLACK, "THD:");

    /* 谐波标签 H1..H5 */
    for (int k = 0; k < THD_MAX_HARMONIC; ++k) {
        LCD_DrawTextf(4, THD_H1_Y + k * THD_ROW_DY, WHITE, BLACK,
                      "H%d", k + 1);
    }

    /* 清一次上一帧条形宽度记录 */
    for (int k = 0; k < THD_MAX_HARMONIC; ++k) s_last_bar_w[k] = 0;
}

static void thd_tick(void)
{
    const thd_result_t *r = thd_get_result();

    /* 第一行：f0 / THD */
    LCD_DrawTextf(  30,  30, GREEN, BLACK, "%6.1f Hz  ", (double)r->f0_hz);
    LCD_DrawTextf( 190,  30, r->thd_percent > 5.0f ? RED : GREEN, BLACK,
                   "%5.2f %%  ", (double)r->thd_percent);

    /* 谐波条：每根条宽度 = H[k] / H1 × THD_BAR_MAX_W */
    float h1 = r->harmonic[0];
    if (h1 < 1e-3f) h1 = 1.0f;   /* 无信号时的兜底 */

    for (int k = 0; k < THD_MAX_HARMONIC; ++k) {
        float ratio = r->harmonic[k] / h1;
        if (ratio > 1.0f) ratio = 1.0f;
        int   w   = (int)(ratio * THD_BAR_MAX_W);
        int   y   = THD_H1_Y + k * THD_ROW_DY;

        /* 差量重画：只擦掉比上次多余的部分 + 补画新增部分 */
        if (w < s_last_bar_w[k]) {
            /* 变短 → 擦掉右侧多出来 */
            LCD_FillRect(THD_BAR_X + w, y,
                         s_last_bar_w[k] - w, THD_BAR_H, BLACK);
        } else if (w > s_last_bar_w[k]) {
            /* 变长 → 补画右侧新增 */
            LCD_FillRect(THD_BAR_X + s_last_bar_w[k], y,
                         w - s_last_bar_w[k], THD_BAR_H, GREEN);
        }
        s_last_bar_w[k] = w;

        /* 数值：H[k] 绝对值（原始幅度） */
        LCD_DrawTextf(THD_VAL_X, y, WHITE, BLACK, "%7.1f", (double)r->harmonic[k]);
    }

    /* 底部状态栏 */
    LCD_DrawTextf(4, 210, WHITE, BLACK,
                  "#%lu  Fs=%lu Hz  %s   ",
                  (unsigned long)r->frame_id,
                  (unsigned long)r->fs_hz,
                  thd_is_running() ? "RUN " : "STOP");
}

static const ui_page_t s_thd_page = {
    .name  = "thd",
    .enter = thd_enter,
    .tick  = thd_tick,
};

/* ============================================================
 *  Page registration & switching
 * ============================================================ */

void ui_register_page(const ui_page_t *p)
{
    if (!p || s_page_count >= UI_MAX_PAGES) return;
    s_pages[s_page_count++] = p;
}

bool ui_switch_to(const char *name)
{
    for (uint8_t i = 0; i < s_page_count; ++i) {
        if (strcmp(s_pages[i]->name, name) == 0) {
            s_current = s_pages[i];
            s_dirty   = true;
            return true;
        }
    }
    return false;
}

/* ============================================================
 *  Terminal 命令：ui.page <name>
 * ============================================================ */

static int cmd_ui_page(int argc, char **argv)
{
    if (argc < 2) {
        term_printf("usage: ui.page <name>\r\n");
        term_printf("pages:");
        for (uint8_t i = 0; i < s_page_count; ++i)
            term_printf(" %s", s_pages[i]->name);
        term_printf("\r\n");
        return -1;
    }
    if (!ui_switch_to(argv[1])) {
        term_printf("unknown page: %s\r\n", argv[1]);
        return -1;
    }
    return 0;
}
static term_cmd_t s_c_ui_page = { "ui.page", cmd_ui_page, "<name> switch page", 0 };

/* ============================================================
 *  Init / Task
 * ============================================================ */

void ui_init(void)
{
    LCD_Init();
    ui_register_page(&s_welcome);
    ui_register_page(&s_thd_page);
    ui_switch_to("welcome");
    term_register(&s_c_ui_page);
}

void ui_task(void)
{
    if (!s_current) return;
    if (s_dirty) {
        if (s_current->enter) s_current->enter();
        s_dirty = false;
    }
    if (s_current->tick) s_current->tick();
}
