#ifndef _MAIN_C_
#error "NOTE: this header may only be included by main.c"
#endif

#ifndef _MAIN_H_
#define _MAIN_H_

#include "main_common.h"
#include <ti/devices/msp432e4/driverlib/driverlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#include "adc.h"
#include "esp8266.h"
#include "goertzel.h"
#include "printf.h"
#include "bsp_sdwa.h"
#include "uartstdio.h"
#include "arm_fft.h"

uint32_t systemClock;
uint32_t PRINTF_UART_BASE = UART0_BASE;
uint8_t g_instr;

float Uo[5]={0.0};
float THDx = 0.0;
uint32_t BaseFreq = 0;
uint32_t BaseFreqDisp = 0;
uint32_t SampleFreq = 0;

bool adcDone = false;
uint16_t adcBuffer[NPT];
float32_t fft_in[NPT], fft_out[NPT];
uint32_t base_freq_index;

uint16_t pBuffer[PBUFFERSIZE]={0};

CTRL_FLAGS flags = {0};


uint8_t tcp_link = 0;            //手机连接成功标志
uint8_t tcp_start[5] = {0};                   //检测启动标志

#endif
