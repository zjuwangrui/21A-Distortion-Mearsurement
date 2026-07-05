#include "lcd.h"
#include <stdio.h>
#include <string.h>

/* ===== 布局（横屏 320×240，Font8x16，行高 18px）
 *
 * 分组：每组 值/阈值/状态 连续排列，组间 8px 空隙，WiFi 前 14px 空隙
 *
 * 标题         0-23
 * ── Voltage 组 ──
 * V 值         26
 * V 阈值       44
 * V 状态       62
 * [8px gap]
 * ── Light 组 ──
 * L 值         86
 * L 阈值      104
 * L 状态      122
 * [8px gap]
 * ── Temp 组 ──
 * T 值        146
 * T 阈值      164
 * T 状态      182
 * [14px gap]
 * WiFi        212
 * ===== */
#define ROW_V_Y      26
#define ROW_V_THR_Y  44
#define ROW_VS_Y     62

#define ROW_L_Y      86
#define ROW_L_THR_Y 104
#define ROW_LS_Y    122

#define ROW_T_Y     146
#define ROW_T_THR_Y 164
#define ROW_TS_Y    182

#define ROW_WIFI_Y  212

#define COL_LABEL     4
#define COL_VALUE   140

static void draw_title(void)
{
    LCD_SetColors(WHITE, BLUE);
    LCD_SetFont(&Font8x16);
    ILI9341_Clear(0, 0, LCD_X_LENGTH, 22);
    ILI9341_DispString_EN(20, 4, "VTL Sensor System");
}

static void draw_static_labels(void)
{
    LCD_SetColors(BLACK, WHITE);
    LCD_SetFont(&Font8x16);

    ILI9341_DispString_EN(COL_LABEL, ROW_V_Y,     "Voltage  :");
    ILI9341_DispString_EN(COL_LABEL, ROW_V_THR_Y, "V-Thresh :");
    ILI9341_DispString_EN(COL_LABEL, ROW_VS_Y,    "V-Status :");

    ILI9341_DispString_EN(COL_LABEL, ROW_L_Y,     "Light    :");
    ILI9341_DispString_EN(COL_LABEL, ROW_L_THR_Y, "L-Thresh :");
    ILI9341_DispString_EN(COL_LABEL, ROW_LS_Y,    "L-Status :");

    ILI9341_DispString_EN(COL_LABEL, ROW_T_Y,     "Temp     :");
    ILI9341_DispString_EN(COL_LABEL, ROW_T_THR_Y, "T-Thresh :");
    ILI9341_DispString_EN(COL_LABEL, ROW_TS_Y,    "T-Status :");
}

void LCD_DisplayInit(void)
{
    ILI9341_Init();
    ILI9341_GramScan(6);
    ILI9341_Clear(0, 0, LCD_X_LENGTH, LCD_Y_LENGTH);
    ILI9341_BackLed_Control(ENABLE);
    draw_title();
    draw_static_labels();
}

void LCD_ShowBoot(void)
{
    LCD_SetColors(YELLOW, WHITE);
    LCD_SetFont(&Font8x16);
    ILI9341_Clear(COL_LABEL, ROW_WIFI_Y, LCD_X_LENGTH - COL_LABEL, 18);
    ILI9341_DispString_EN(COL_LABEL, ROW_WIFI_Y, "WiFi: Initializing...");
}

void LCD_ShowESPFail(void)
{
    ILI9341_Clear(0, 0, LCD_X_LENGTH, LCD_Y_LENGTH);
    LCD_SetColors(WHITE, RED);
    LCD_SetFont(&Font16x24);
    ILI9341_DispString_EN(30,  80, "ESP8266 INIT");
    ILI9341_DispString_EN(60, 110, "  FAILED  ");
    LCD_SetFont(&Font8x16);
    ILI9341_DispString_EN(20, 150, "Check wiring & power");
}

static void draw_status_row(uint16_t y, int8_t over)
{
    ILI9341_Clear(COL_VALUE, y, LCD_X_LENGTH - COL_VALUE, 16);
    LCD_SetFont(&Font8x16);
    if (over) {
        LCD_SetColors(RED, WHITE);
        ILI9341_DispString_EN(COL_VALUE, y, "OVER  ");
    } else {
        LCD_SetColors(GREEN, WHITE);
        ILI9341_DispString_EN(COL_VALUE, y, "NORMAL");
    }
}

void LCD_UpdateSensor(float voltage, float v_thr,
                      uint32_t light, uint32_t l_thr,
                      uint8_t temp, uint8_t temp_thr)
{
    char buf[24];
    LCD_SetFont(&Font8x16);

    /* ── Voltage 组 ── */
    LCD_SetColors(BLUE, WHITE);
    ILI9341_Clear(COL_VALUE, ROW_V_Y, LCD_X_LENGTH - COL_VALUE, 16);
    snprintf(buf, sizeof(buf), "%.3f V", voltage);
    ILI9341_DispString_EN(COL_VALUE, ROW_V_Y, buf);

    static float last_v_thr = -1.0f;
    if (v_thr != last_v_thr) {
        last_v_thr = v_thr;
        LCD_SetColors(BLACK, WHITE);
        ILI9341_Clear(COL_VALUE, ROW_V_THR_Y, LCD_X_LENGTH - COL_VALUE, 16);
        snprintf(buf, sizeof(buf), "%.3f V", v_thr);
        ILI9341_DispString_EN(COL_VALUE, ROW_V_THR_Y, buf);
    }

    static int8_t last_v_over = -1;
    int8_t v_over = (voltage > v_thr) ? 1 : 0;
    if (v_over != last_v_over) { last_v_over = v_over; draw_status_row(ROW_VS_Y, v_over); }

    /* ── Light 组 ── */
    LCD_SetColors(BLUE, WHITE);
    ILI9341_Clear(COL_VALUE, ROW_L_Y, LCD_X_LENGTH - COL_VALUE, 16);
    snprintf(buf, sizeof(buf), "%4lu", (unsigned long)light);
    ILI9341_DispString_EN(COL_VALUE, ROW_L_Y, buf);

    static uint32_t last_l_thr = 0xFFFFFFFFU;
    if (l_thr != last_l_thr) {
        last_l_thr = l_thr;
        LCD_SetColors(BLACK, WHITE);
        ILI9341_Clear(COL_VALUE, ROW_L_THR_Y, LCD_X_LENGTH - COL_VALUE, 16);
        snprintf(buf, sizeof(buf), "%4lu", (unsigned long)l_thr);
        ILI9341_DispString_EN(COL_VALUE, ROW_L_THR_Y, buf);
    }

    static int8_t last_l_over = -1;
    int8_t l_over = (light > l_thr) ? 1 : 0;
    if (l_over != last_l_over) { last_l_over = l_over; draw_status_row(ROW_LS_Y, l_over); }

    /* ── Temp 组 ── */
    LCD_SetColors(BLUE, WHITE);
    ILI9341_Clear(COL_VALUE, ROW_T_Y, LCD_X_LENGTH - COL_VALUE, 16);
    snprintf(buf, sizeof(buf), "%d C", (int)temp);
    ILI9341_DispString_EN(COL_VALUE, ROW_T_Y, buf);

    static uint8_t last_t_thr = 0xFFU;
    if (temp_thr != last_t_thr) {
        last_t_thr = temp_thr;
        LCD_SetColors(BLACK, WHITE);
        ILI9341_Clear(COL_VALUE, ROW_T_THR_Y, LCD_X_LENGTH - COL_VALUE, 16);
        snprintf(buf, sizeof(buf), "%d C", (int)temp_thr);
        ILI9341_DispString_EN(COL_VALUE, ROW_T_THR_Y, buf);
    }

    static int8_t last_t_over = -1;
    int8_t t_over = (temp > temp_thr) ? 1 : 0;
    if (t_over != last_t_over) { last_t_over = t_over; draw_status_row(ROW_TS_Y, t_over); }
}

void LCD_ShowWiFiStatus(bool esp_ok)
{
    ILI9341_Clear(COL_LABEL, ROW_WIFI_Y, LCD_X_LENGTH - COL_LABEL, 18);
    LCD_SetFont(&Font8x16);
    if (esp_ok) {
        LCD_SetColors(GREEN, WHITE);
        ILI9341_DispString_EN(COL_LABEL, ROW_WIFI_Y, "WiFi:OK  Green LED=init OK");
    } else {
        LCD_SetColors(RED, WHITE);
        ILI9341_DispString_EN(COL_LABEL, ROW_WIFI_Y, "WiFi: FAIL");
    }
}
