#include "module/ui.h"
#include "bsp/lcd.h"
#include "stm32f1xx_hal.h"
#include <string.h>

#define UI_MAX_PAGES  4

static const ui_page_t *s_pages[UI_MAX_PAGES];
static uint8_t          s_page_count = 0;
static const ui_page_t *s_current    = 0;
static bool             s_dirty      = true;   /* 需要 enter() 一次 */

/* ---------- 默认欢迎页 ---------- */

static void welcome_enter(void)
{
    LCD_Clear(BLACK);
    LCD_DrawTextC( 40,  30, YELLOW, BLACK, "STM32 App Framework");
    LCD_DrawTextC( 40,  55, WHITE,  BLACK, "----------------------------");
    LCD_DrawTextC( 40,  80, WHITE,  BLACK, "core   : scheduler + ring");
    LCD_DrawTextC( 40,  98, WHITE,  BLACK, "module : terminal + fft + ui");
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

/* ---------- 注册与切换 ---------- */

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

void ui_init(void)
{
    LCD_Init();
    ui_register_page(&s_welcome);
    ui_switch_to("welcome");
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
