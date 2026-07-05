#ifndef __LCD_H
#define __LCD_H

#include <stdbool.h>
#include <stdint.h>
#include "bsp_ili9341_lcd.h"

void LCD_DisplayInit(void);
void LCD_ShowBoot(void);
void LCD_ShowESPFail(void);
void LCD_UpdateSensor(float voltage, float v_thr,
                      uint32_t light, uint32_t l_thr,
                      uint8_t temp, uint8_t temp_thr);
void LCD_ShowWiFiStatus(bool esp_ok);

#endif /* __LCD_H */
