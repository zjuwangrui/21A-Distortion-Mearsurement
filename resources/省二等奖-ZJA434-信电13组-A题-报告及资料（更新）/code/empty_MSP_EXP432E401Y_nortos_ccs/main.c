/* --COPYRIGHT--,BSD
 * Copyright (c) 2017, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * --/COPYRIGHT--*/
/******************************************************************************
 * MSP432E4 Empty Project
 *
 * Description: An empty project that uses DriverLib
 *
 *                MSP432E401Y
 *             ------------------
 *         /|\|                  |
 *          | |                  |
 *          --|RST               |
 *            |                  |
 *            |                  |
 *            |                  |
 *            |                  |
 *            |                  |
 * Author: 
*******************************************************************************/
/* DriverLib Includes */
#include <ti/devices/msp432e4/driverlib/driverlib.h>

/* Standard Includes */
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include "./window/inc/window.h"
#include "./Goertzel/Goertzel.h"
/* Display Include via console */
#include "uartstdio.h"

/* The control table used by the uDMA controller.  This table must be aligned
 * to a 1024 byte boundary. */
#if defined(__ICCARM__)
#pragma data_alignment=1024
uint8_t pui8ControlTable[1024];
#elif defined(__TI_ARM__)
#pragma DATA_ALIGN(pui8ControlTable, 1024)
uint8_t pui8ControlTable[1024];
#else
uint8_t pui8ControlTable[1024] __attribute__ ((aligned(1024)));
#endif

uint16_t ADCBuffer[1024];

#define MAXSIZE 64
#define AMP 4.25

int32_t *getADCBuffer(int32_t index);
double *getVRBuffer(int32_t index);
double *getVIBuffer(int32_t index);
double *getFFTRBuffer(int32_t index);
double *getFFTIBuffer(int32_t index);
void kfft(int32_t n, int32_t k);




double sig_out[128];
double sig_max = -4096;
double sig_min = 4096;
double sig_pp = 0;

int32_t ADCBuffer0[MAXSIZE];
int32_t ADCBuffer1[MAXSIZE];
int32_t ADCBuffer2[MAXSIZE];
int32_t ADCBuffer3[MAXSIZE];
int32_t ADCBuffer4[MAXSIZE];
int32_t ADCBuffer5[MAXSIZE];
int32_t ADCBuffer6[MAXSIZE];
int32_t ADCBuffer7[MAXSIZE];
int32_t ADCBuffer8[MAXSIZE];
int32_t ADCBuffer9[MAXSIZE];
int32_t ADCBuffera[MAXSIZE];
int32_t ADCBufferb[MAXSIZE];
int32_t ADCBufferc[MAXSIZE];
int32_t ADCBufferd[MAXSIZE];
int32_t ADCBuffere[MAXSIZE];
int32_t ADCBufferf[MAXSIZE];

double VRBuffer0[MAXSIZE];
double VRBuffer1[MAXSIZE];
double VRBuffer2[MAXSIZE];
double VRBuffer3[MAXSIZE];
double VRBuffer4[MAXSIZE];
double VRBuffer5[MAXSIZE];
double VRBuffer6[MAXSIZE];
double VRBuffer7[MAXSIZE];
double VRBuffer8[MAXSIZE];
double VRBuffer9[MAXSIZE];
double VRBuffera[MAXSIZE];
double VRBufferb[MAXSIZE];
double VRBufferc[MAXSIZE];
double VRBufferd[MAXSIZE];
double VRBuffere[MAXSIZE];
double VRBufferf[MAXSIZE];

double VIBuffer0[MAXSIZE];
double VIBuffer1[MAXSIZE];
double VIBuffer2[MAXSIZE];
double VIBuffer3[MAXSIZE];
double VIBuffer4[MAXSIZE];
double VIBuffer5[MAXSIZE];
double VIBuffer6[MAXSIZE];
double VIBuffer7[MAXSIZE];
double VIBuffer8[MAXSIZE];
double VIBuffer9[MAXSIZE];
double VIBuffera[MAXSIZE];
double VIBufferb[MAXSIZE];
double VIBufferc[MAXSIZE];
double VIBufferd[MAXSIZE];
double VIBuffere[MAXSIZE];
double VIBufferf[MAXSIZE];

double FFTRBuffer0[MAXSIZE];
double FFTRBuffer1[MAXSIZE];
double FFTRBuffer2[MAXSIZE];
double FFTRBuffer3[MAXSIZE];
double FFTRBuffer4[MAXSIZE];
double FFTRBuffer5[MAXSIZE];
double FFTRBuffer6[MAXSIZE];
double FFTRBuffer7[MAXSIZE];
double FFTRBuffer8[MAXSIZE];
double FFTRBuffer9[MAXSIZE];
double FFTRBuffera[MAXSIZE];
double FFTRBufferb[MAXSIZE];
double FFTRBufferc[MAXSIZE];
double FFTRBufferd[MAXSIZE];
double FFTRBuffere[MAXSIZE];
double FFTRBufferf[MAXSIZE];

double FFTIBuffer0[MAXSIZE];
double FFTIBuffer1[MAXSIZE];
double FFTIBuffer2[MAXSIZE];
double FFTIBuffer3[MAXSIZE];
double FFTIBuffer4[MAXSIZE];
double FFTIBuffer5[MAXSIZE];
double FFTIBuffer6[MAXSIZE];
double FFTIBuffer7[MAXSIZE];
double FFTIBuffer8[MAXSIZE];
double FFTIBuffer9[MAXSIZE];
double FFTIBuffera[MAXSIZE];
double FFTIBufferb[MAXSIZE];
double FFTIBufferc[MAXSIZE];
double FFTIBufferd[MAXSIZE];
double FFTIBuffere[MAXSIZE];
double FFTIBufferf[MAXSIZE];

uint32_t getDCVoltage;

double v_ave = 0;
double vmin = 3.3;
double vmax = 0;
double vpp = 0;
uint32_t vmax_i = 0;

double magmax = 0;
uint32_t magmax_i = 0;

double f_test[21];
double mag_test[21];
double mag_test_max;
uint32_t mag_test_i;

double f_in;
double THD;
double f_out;
double THD_out;
double mag_out[5];

double v_dc = 0;

double vv_temp = 5;

double mag_amp = 0;

volatile uint16_t transfer_flag = 0;
uint16_t transfer_count = 0;

uint16_t circle_count = 0;
uint8_t uart_temp;

double sample_rate = 1000000.0;

int32_t *getADCBuffer(int32_t index)
{
    uint32_t temp;
    temp = index / MAXSIZE;
    switch(temp){
        case 0:
            return &ADCBuffer0[index];
        case 1:
            return &ADCBuffer1[index - temp * MAXSIZE];
        case 2:
            return &ADCBuffer2[index - temp * MAXSIZE];
        case 3:
            return &ADCBuffer3[index - temp * MAXSIZE];
        case 4:
            return &ADCBuffer4[index - temp * MAXSIZE];
        case 5:
            return &ADCBuffer5[index - temp * MAXSIZE];
        case 6:
            return &ADCBuffer6[index - temp * MAXSIZE];
        case 7:
            return &ADCBuffer7[index - temp * MAXSIZE];
        case 8:
            return &ADCBuffer8[index - temp * MAXSIZE];
        case 9:
            return &ADCBuffer9[index - temp * MAXSIZE];
        case 10:
            return &ADCBuffera[index - temp * MAXSIZE];
        case 11:
            return &ADCBufferb[index - temp * MAXSIZE];
        case 12:
            return &ADCBufferc[index - temp * MAXSIZE];
        case 13:
            return &ADCBufferd[index - temp * MAXSIZE];
        case 14:
            return &ADCBuffere[index - temp * MAXSIZE];
        case 15:
            return &ADCBufferf[index - temp * MAXSIZE];
    }
    return &ADCBuffer0[0];
}

double *getVRBuffer(int32_t index)
{
    uint32_t temp;
    temp = index / MAXSIZE;
    switch(temp){
        case 0:
            return &VRBuffer0[index];
        case 1:
            return &VRBuffer1[index - temp * MAXSIZE];
        case 2:
            return &VRBuffer2[index - temp * MAXSIZE];
        case 3:
            return &VRBuffer3[index - temp * MAXSIZE];
        case 4:
            return &VRBuffer4[index - temp * MAXSIZE];
        case 5:
            return &VRBuffer5[index - temp * MAXSIZE];
        case 6:
            return &VRBuffer6[index - temp * MAXSIZE];
        case 7:
            return &VRBuffer7[index - temp * MAXSIZE];
        case 8:
            return &VRBuffer8[index - temp * MAXSIZE];
        case 9:
            return &VRBuffer9[index - temp * MAXSIZE];
        case 10:
            return &VRBuffera[index - temp * MAXSIZE];
        case 11:
            return &VRBufferb[index - temp * MAXSIZE];
        case 12:
            return &VRBufferc[index - temp * MAXSIZE];
        case 13:
            return &VRBufferd[index - temp * MAXSIZE];
        case 14:
            return &VRBuffere[index - temp * MAXSIZE];
        case 15:
            return &VRBufferf[index - temp * MAXSIZE];
    }
    return &VRBuffer0[0];
}

double *getVIBuffer(int32_t index)
{
    uint32_t temp;
    temp = index / MAXSIZE;
    switch(temp){
        case 0:
            return &VIBuffer0[index];
        case 1:
            return &VIBuffer1[index - temp * MAXSIZE];
        case 2:
            return &VIBuffer2[index - temp * MAXSIZE];
        case 3:
            return &VIBuffer3[index - temp * MAXSIZE];
        case 4:
            return &VIBuffer4[index - temp * MAXSIZE];
        case 5:
            return &VIBuffer5[index - temp * MAXSIZE];
        case 6:
            return &VIBuffer6[index - temp * MAXSIZE];
        case 7:
            return &VIBuffer7[index - temp * MAXSIZE];
        case 8:
            return &VIBuffer8[index - temp * MAXSIZE];
        case 9:
            return &VIBuffer9[index - temp * MAXSIZE];
        case 10:
            return &VIBuffera[index - temp * MAXSIZE];
        case 11:
            return &VIBufferb[index - temp * MAXSIZE];
        case 12:
            return &VIBufferc[index - temp * MAXSIZE];
        case 13:
            return &VIBufferd[index - temp * MAXSIZE];
        case 14:
            return &VIBuffere[index - temp * MAXSIZE];
        case 15:
            return &VIBufferf[index - temp * MAXSIZE];
    }
    return &VIBuffer0[0];
}

double *getFFTRBuffer(int32_t index)
{
    uint32_t temp;
    temp = index / MAXSIZE;
    switch(temp){
        case 0:
            return &FFTRBuffer0[index];
        case 1:
            return &FFTRBuffer1[index - temp * MAXSIZE];
        case 2:
            return &FFTRBuffer2[index - temp * MAXSIZE];
        case 3:
            return &FFTRBuffer3[index - temp * MAXSIZE];
        case 4:
            return &FFTRBuffer4[index - temp * MAXSIZE];
        case 5:
            return &FFTRBuffer5[index - temp * MAXSIZE];
        case 6:
            return &FFTRBuffer6[index - temp * MAXSIZE];
        case 7:
            return &FFTRBuffer7[index - temp * MAXSIZE];
        case 8:
            return &FFTRBuffer8[index - temp * MAXSIZE];
        case 9:
            return &FFTRBuffer9[index - temp * MAXSIZE];
        case 10:
            return &FFTRBuffera[index - temp * MAXSIZE];
        case 11:
            return &FFTRBufferb[index - temp * MAXSIZE];
        case 12:
            return &FFTRBufferc[index - temp * MAXSIZE];
        case 13:
            return &FFTRBufferd[index - temp * MAXSIZE];
        case 14:
            return &FFTRBuffere[index - temp * MAXSIZE];
        case 15:
            return &FFTRBufferf[index - temp * MAXSIZE];
    }
    return &FFTRBuffer0[0];
}

double *getFFTIBuffer(int32_t index)
{
    uint32_t temp;
    temp = index / MAXSIZE;
    switch(temp){
        case 0:
            return &FFTIBuffer0[index];
        case 1:
            return &FFTIBuffer1[index - temp * MAXSIZE];
        case 2:
            return &FFTIBuffer2[index - temp * MAXSIZE];
        case 3:
            return &FFTIBuffer3[index - temp * MAXSIZE];
        case 4:
            return &FFTIBuffer4[index - temp * MAXSIZE];
        case 5:
            return &FFTIBuffer5[index - temp * MAXSIZE];
        case 6:
            return &FFTIBuffer6[index - temp * MAXSIZE];
        case 7:
            return &FFTIBuffer7[index - temp * MAXSIZE];
        case 8:
            return &FFTIBuffer8[index - temp * MAXSIZE];
        case 9:
            return &FFTIBuffer9[index - temp * MAXSIZE];
        case 10:
            return &FFTIBuffera[index - temp * MAXSIZE];
        case 11:
            return &FFTIBufferb[index - temp * MAXSIZE];
        case 12:
            return &FFTIBufferc[index - temp * MAXSIZE];
        case 13:
            return &FFTIBufferd[index - temp * MAXSIZE];
        case 14:
            return &FFTIBuffere[index - temp * MAXSIZE];
        case 15:
            return &FFTIBufferf[index - temp * MAXSIZE];
    }
    return &FFTIBuffer0[0];
}

//void UDMA_IRQHandler(void)
//{
//    transfer_count++;
//    if(transfer_count <= 15){
//        MAP_uDMAChannelTransferSet(UDMA_CH17_ADC0_3 | UDMA_PRI_SELECT,
//                                   UDMA_MODE_BASIC,
//                                   (void *)&ADC0->SSFIFO3, (void *)ADCBuffer0,
//                                   64);
//        MAP_uDMAChannelEnable(UDMA_CH17_ADC0_3);
//    }
//    else{
//        transfer_count = 0;
//        transfer_flag = 1;
//        MAP_uDMAChannelDisable(UDMA_CH17_ADC0_3);
//        MAP_uDMAChannelTransferSet(UDMA_CH17_ADC0_3 | UDMA_PRI_SELECT,
//                                   UDMA_MODE_BASIC,
//                                   (void *)&ADC0->SSFIFO3, (void *)ADCBuffer0,
//                                   64);
//    }
//}
void kfft(int32_t n,int32_t k)
{
  int32_t it,m,is,i,j,nv,l0;
  double p,q,s,vr,vi,poddr,poddi;
  vmin = 3.3;
  vmax = 0;
  for (it = 0; it <= n-1; it++)  //将pr[0]和pi[0]循环赋值给fr[]和fi[]
  {
      m=it;
      is=0;
      for(i=0; i<=k-1; i++)
      {
          j=m/2;
          is=2*is+(m-2*j);
          m=j;
      }
      *(getFFTRBuffer(it)) = (double)(*(getADCBuffer(is))  - v_ave) * 3.3 / 4096.0;
      *(getFFTIBuffer(it)) = 0;
      if(vmin > *(getFFTRBuffer(it))) vmin = *(getFFTRBuffer(it));
      if(vmax < *(getFFTRBuffer(it))){
          vmax = *(getFFTRBuffer(it));
          vmax_i = is;
      }
      *(getFFTRBuffer(it)) = *(getFFTRBuffer(it))  * hanning_1024[is];
  }
  *(getVRBuffer(0)) = 1.0;
  *(getVIBuffer(0)) = 0.0;
  p = 6.283185306 / (1.0 * n);
  *(getVRBuffer(1)) = cos(p); //将w=e^-j2pi/n用欧拉公式表示
  *(getVIBuffer(1)) = -sin(p);

  for (i=2; i<=n-1; i++)  //计算pr[]
  {
      p = *(getVRBuffer(i-1)) * *(getVRBuffer(1));
      q = *(getVIBuffer(i-1)) * *(getVIBuffer(1));
      s = (*(getVRBuffer(i-1)) + *(getVIBuffer(i-1)))*(*(getVRBuffer(1)) + *(getVIBuffer(1)));
      *(getVRBuffer(i)) = p - q;
      *(getVIBuffer(i)) = s - p - q;
  }

  for (it=0; it<=n-2; it=it+2)
  {
      vr = *(getFFTRBuffer(it));
      vi = *(getFFTIBuffer(it));
      *(getFFTRBuffer(it)) = vr + *(getFFTRBuffer(it + 1));
      *(getFFTIBuffer(it)) = vi + *(getFFTIBuffer(it + 1));
      *(getFFTRBuffer(it+1)) = vr - *(getFFTRBuffer(it + 1));
      *(getFFTIBuffer(it+1)) = vi - *(getFFTIBuffer(it + 1));
  }

  m=n/2;
  nv=2;
  for (l0=k-2; l0>=0; l0--) //蝴蝶操作
  {
      m=m/2;
      nv=2*nv;
      for (it=0; it<=(m-1)*nv; it=it+nv)
        for (j=0; j<=(nv/2)-1; j++)
          {
              p = *(getVRBuffer(m*j)) * *(getFFTRBuffer(it+j+nv/2));
              q = *(getVIBuffer(m*j)) * *(getFFTIBuffer(it+j+nv/2));
              s = *(getVRBuffer(m*j)) + *(getVIBuffer(m*j));
              s = s * (*(getFFTRBuffer(it+j+nv/2)) + *(getFFTIBuffer(it+j+nv/2)));
              poddr = p - q;
              poddi = s - p - q;
              *(getFFTRBuffer(it+j+nv/2)) = *(getFFTRBuffer(it+j)) - poddr;
              *(getFFTIBuffer(it+j+nv/2)) = *(getFFTIBuffer(it+j)) - poddi;
              *(getFFTRBuffer(it+j)) = *(getFFTRBuffer(it+j)) + poddr;
              *(getFFTIBuffer(it+j)) = *(getFFTIBuffer(it+j)) + poddi;
          }
  }
  magmax_i = 0;
  magmax = 0;
  for (i=0; i<=n/2-1; i++){
    *(getVRBuffer(i)) = sqrt(*(getFFTRBuffer(i)) * *(getFFTRBuffer(i)) + *(getFFTIBuffer(i)) * *(getFFTIBuffer(i)));  //幅值计算
    if(i >= 1 && magmax < *(getVRBuffer(i))){
      magmax =  *(getVRBuffer(i));
      magmax_i = i;
    }
 }
  return;
}

void ADC0SS3_IRQHandler(void)
{
    uint32_t getIntStatus;

    /* Get the interrupt status from the ADC */
    getIntStatus = MAP_ADCIntStatusEx(ADC0_BASE, true);

    MAP_IntDisable(INT_ADC0SS3);

    MAP_ADCIntDisableEx(ADC0_BASE, ADC_INT_DMA_SS3);
    /* If the interrupt status for Sequencer-2 is set the
     * clear the status and read the data */
    if((getIntStatus & ADC_INT_DMA_SS3) == ADC_INT_DMA_SS3)
    {
        transfer_flag++;
        /* Clear the ADC interrupt flag. */
        if(transfer_flag == 1024){
            MAP_uDMAChannelDisable(UDMA_CH17_ADC0_3);
        }
        MAP_ADCIntClearEx(ADC0_BASE, ADC_INT_DMA_SS3);
        MAP_ADCIntClear(ADC0_BASE, 3);

    }
}

void ConfigureUART2(uint32_t systemClock)
{
    /* Enable the clock to GPIO port A and UART 0 */
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART2);

    /* Configure the GPIO Port A for UART 0 */
    MAP_GPIOPinConfigure(GPIO_PD4_U2RX);
    MAP_GPIOPinConfigure(GPIO_PD5_U2TX);
    MAP_GPIOPinTypeUART(GPIO_PORTD_BASE, GPIO_PIN_4 | GPIO_PIN_5);

    /* Configure the UART for 115200 bps 8-N-1 format */

}

void ConfigureUART7(uint32_t systemClock)
{
    /* Enable the clock to GPIO port A and UART 0 */
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART7);

    /* Configure the GPIO Port A for UART 0 */
    MAP_GPIOPinConfigure(GPIO_PC4_U7RX);
    MAP_GPIOPinConfigure(GPIO_PC5_U7TX);
    MAP_GPIOPinTypeUART(GPIO_PORTC_BASE, GPIO_PIN_4 | GPIO_PIN_5);

    /* Configure the UART for 115200 bps 8-N-1 format */

}

int main(void)
{
    int32_t i = 0;
    double v_out_temp;
    uint32_t systemClock;


    /* Configure the system clock for 120 MHz */
    systemClock = MAP_SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN |
                                          SYSCTL_USE_PLL | SYSCTL_CFG_VCO_480),
                                          120000000);
    /* Initialize serial console */
    ConfigureUART2(systemClock);
    ConfigureUART7(systemClock);
    UARTStdioConfig(7, 38400, systemClock);
    UARTStdioConfig(2, 9600, systemClock);

    /* Enable the clock to GPIO Port N and wait for it to be ready */
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);
    while(!(MAP_SysCtlPeripheralReady(SYSCTL_PERIPH_GPION)))
    {
    }

    /* Configure PN0 as Output for controlling the LED */
    MAP_GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_0);
    MAP_GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_0, ~(GPIO_PIN_0));

    /* Enable the clock to GPIO Port E and wait for it to be ready */
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
    while(!(MAP_SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOE)))
    {
    }

    /* Configure PE3 as ADC input channel */
    MAP_GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_2|GPIO_PIN_3);


    /* Enable the clock to ADC-1 and wait for it to be ready */
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC1);
    while(!(MAP_SysCtlPeripheralReady(SYSCTL_PERIPH_ADC1)))
    {
    }

    /* Configure Sequencer 3 to sample the analog channel : AIN1. The
     * end of conversion and interrupt generation is set for AIN1 */
    MAP_ADCSequenceStepConfigure(ADC1_BASE, 3, 0, ADC_CTL_CH1 | ADC_CTL_IE |
                                 ADC_CTL_END);

    /* Enable sample sequence 3 with a timer trigger.  Sequence 3 will do a
     * single sample when the timer sends a signal to start the conversion */
    MAP_ADCSequenceConfigure(ADC1_BASE, 3, ADC_TRIGGER_PROCESSOR, 0);

    /* Since sample sequence 32 is now configured, it must be enabled. */
    MAP_ADCSequenceEnable(ADC1_BASE, 3);

    /* Clear the interrupt status flag.  This is done to make sure the
     * interrupt flag is cleared before we sample. */
    MAP_ADCIntClear(ADC1_BASE, 3);

    /* Enable the clock to ADC-0 and wait for it to be ready */
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);
    while(!(MAP_SysCtlPeripheralReady(SYSCTL_PERIPH_ADC0)))
    {
    }


    /* Configure Sequencer 3 to sample a single analog channel : AIN0 */
    MAP_ADCSequenceStepConfigure(ADC0_BASE, 3, 0, ADC_CTL_CH0 | ADC_CTL_IE |
                                 ADC_CTL_END);

    /* Enable sample sequence 3 with a timer trigger.  Sequence 3 will do a
     * single sample when the timer sends a signal to start the conversion */
    MAP_ADCSequenceConfigure(ADC0_BASE, 3, ADC_TRIGGER_TIMER, 3);

    /* Clear the interrupt status flag before enabling. This is done to make
     * sure the interrupt flag is cleared before we sample. */
    MAP_ADCIntClearEx(ADC0_BASE, ADC_INT_DMA_SS3);
    MAP_ADCIntEnableEx(ADC0_BASE, ADC_INT_DMA_SS3);

    /* Clear the interrupt status flag.  This is done to make sure the
     * interrupt flag is cleared before we sample. */
    MAP_ADCIntClear(ADC0_BASE, 3);

    /* Enable the DMA request from ADC0 Sequencer 3 */
    MAP_ADCSequenceDMAEnable(ADC0_BASE, 3);

    /* Since sample sequence 3 is now configured, it must be enabled. */
    MAP_ADCSequenceEnable(ADC0_BASE, 3);


    /* Enable the DMA and Configure Channel for TIMER0A for Ping Pong mode of
     * transfer */
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_UDMA);
    while(!(SysCtlPeripheralReady(SYSCTL_PERIPH_UDMA)))
    {
    }

    MAP_uDMAEnable();

    /* Point at the control table to use for channel control structures. */
    MAP_uDMAControlBaseSet(pui8ControlTable);

    /* Map the ADC0 Sequencer 3 DMA channel */
    MAP_uDMAChannelAssign(UDMA_CH17_ADC0_3);

    /* Put the attributes in a known state for the uDMA ADC0 Sequencer 2
     * channel. These should already be disabled by default. */
    MAP_uDMAChannelAttributeDisable(UDMA_CH17_ADC0_3,
                                    UDMA_ATTR_ALTSELECT | UDMA_ATTR_USEBURST |
                                    UDMA_ATTR_HIGH_PRIORITY |
                                    UDMA_ATTR_REQMASK);

    /* Configure the control parameters for the primary control structure for
     * the ADC0 Sequencer 2 channel. The primary control structure is used for
     * copying the data from ADC0 Sequencer 2 FIFO to srcBuffer. The transfer
     * data size is 16 bits and the source address is not incremented while
     * the destination address is incremented at 16-bit boundary.
     */
    MAP_uDMAChannelControlSet(UDMA_CH17_ADC0_3 | UDMA_PRI_SELECT,
                              UDMA_SIZE_16 | UDMA_SRC_INC_NONE | UDMA_DST_INC_16 |
                              UDMA_ARB_1);

    /* Set up the transfer parameters for the ADC0 Sequencer 3 primary control
     * structure. The mode is Basic mode so it will run to completion. */
    MAP_uDMAChannelTransferSet(UDMA_CH17_ADC0_3 | UDMA_PRI_SELECT,
                               UDMA_MODE_BASIC,
                               (void *)&ADC0->SSFIFO3, (void *)ADCBuffer,
                               1024);


    /* Enable Timer-0 clock and configure the timer in periodic mode with
     * a frequency of 1 KHz. Enable the ADC trigger generation from the
     * timer-0. */
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
    while(!(MAP_SysCtlPeripheralReady(SYSCTL_PERIPH_TIMER0)))
    {
    }

    MAP_TimerConfigure(TIMER0_BASE, TIMER_CFG_A_PERIODIC);
    MAP_TimerLoadSet(TIMER0_BASE, TIMER_A, (systemClock/1100000));
    MAP_TimerADCEventSet(TIMER0_BASE, TIMER_ADC_TIMEOUT_A);
    MAP_TimerControlTrigger(TIMER0_BASE, TIMER_A, true);

    while(1)
    {
        if(circle_count == 0){
            uart_temp = MAP_UARTCharGetNonBlocking(UART7_BASE);
            if(uart_temp == 0xAA){
                circle_count = 10;
                MAP_TimerLoadSet(TIMER0_BASE, TIMER_A, (systemClock/1100000)); //初始化采样率
                sample_rate = 1000000.0;
            }
        }
//        if(1){
//            UARTStdioConfig(7, 9600, systemClock);
//            UARTprintf("%c", 0xFF);
//        }
        else{
            if(sample_rate == 1000000 && circle_count == 9 && f_in < 10 * 1000){ //1M采完一次后
                MAP_TimerLoadSet(TIMER0_BASE, TIMER_A, (systemClock/200000)); //切换为200k采样
                sample_rate = 200000.0;

                THD_out = 0;
                mag_out[0] = 0;
                mag_out[1] = 0;
                mag_out[2] = 0;
                mag_out[3] = 0;
                mag_out[4] = 0;
                f_out = 0;
                v_dc = 0;
                circle_count++; //重新开始
            }
            /* The uDMA ADC0 Sequencer 3 channel is primed to start a transfer. As
             * soon as the channel is enabled and the Timer will issue an ADC trigger,
             * the ADC will perform the conversion and send a DMA Request. The data
             * transfers will begin. */
            MAP_TimerEnable(TIMER0_BASE, TIMER_A);

            /* Reconfigure the channel control structure and enable the channel */
            MAP_uDMAChannelTransferSet(UDMA_CH17_ADC0_3 | UDMA_PRI_SELECT,
                                       UDMA_MODE_BASIC,
                                       (void *)&ADC0->SSFIFO3, (void *)ADCBuffer,
                                       1024);
            MAP_uDMAChannelEnable(UDMA_CH17_ADC0_3);

            /* Enable the Interrupt generation from the ADC-0 Sequencer */
            MAP_ADCIntEnableEx(ADC0_BASE, ADC_INT_DMA_SS3);
            MAP_IntEnable(INT_ADC0SS3);

            while(MAP_uDMAChannelIsEnabled(UDMA_CH17_ADC0_3)){

            }

            transfer_flag = 0;
            MAP_TimerDisable(TIMER0_BASE, TIMER_A);

            MAP_uDMAChannelDisable(UDMA_CH17_ADC0_3);

            MAP_IntDisable(INT_ADC0SS3);

            MAP_ADCIntDisableEx(ADC0_BASE, ADC_INT_DMA_SS3);



            for(i = 0; i < 1024; i++){
                *(getADCBuffer(i)) = ADCBuffer[i];
                v_ave = (v_ave * i + ADCBuffer[i]) / (i + 1);
            }
            *(getADCBuffer(0)) = *(getADCBuffer(1));
            kfft(1024, 10);
            vpp = (vmax - vmin) * 0.99;

            mag_test_max = 0;
            mag_test_i = 0;
            if(magmax_i > 1){
                for(i = 0; i < 21; i++){
                    f_test[i] = magmax_i * sample_rate / 1024.0 + (i - 10) * (sample_rate / 20) / 1024.0;
                    mag_test[i] = Goertzel_Mag(cos(6.283185306 * f_test[i] / sample_rate), 0);
                    if(mag_test[i] > mag_test_max){
                        mag_test_max = mag_test[i];
                        mag_test_i = i;
                    }
                }
            }
            else{
                for(i = 11; i < 21; i++){
                    f_test[i] = magmax_i * sample_rate / 1024.0 + (i - 10) * 50000 / 1024.0;
                    mag_test[i] = Goertzel_Mag(cos(6.283185306 * f_test[i] / sample_rate), 0);
                    if(mag_test[i] > mag_test_max){
                        mag_test_max = mag_test[i];
                        mag_test_i = i;
                    }
                }
            }

            f_in = f_test[mag_test_i];
            if(f_in < 1000) f_in = 1000;

            mag_test[0] = Goertzel_Mag(cos(6.283185306 * f_in * 1 / sample_rate), 0) / 1.02; //一次谐波
            mag_test[1] = Goertzel_Mag(cos(6.283185306 * f_in * 2 / sample_rate), 0) / mag_test[0];
            mag_test[2] = Goertzel_Mag(cos(6.283185306 * f_in * 3 / sample_rate), 0) / mag_test[0];
            mag_test[3] = Goertzel_Mag(cos(6.283185306 * f_in * 4 / sample_rate), 0) / mag_test[0];
            mag_test[4] = Goertzel_Mag(cos(6.283185306 * f_in * 5 / sample_rate), 0) / mag_test[0];
            mag_amp = f_in / 1000;
            if(f_in < 10000){
                if(mag_test[1] * vpp < 0.021){
                    mag_test[1] = mag_test[1] * (0.008 * mag_amp * mag_amp * mag_amp - 0.0128 * mag_amp * mag_amp + 0.0625 * mag_amp + 0.99);
                }
                if(mag_test[2] * vpp < 0.021){
                    mag_test[2] = mag_test[2] * (-0.0011 * mag_amp * mag_amp * mag_amp + 0.0181 * mag_amp * mag_amp - 0.0537 * mag_amp + 1.1059);
                }
                if(mag_test[3] * vpp < 0.021){
                    mag_test[3] = mag_test[3] * (-0.0012 * mag_amp * mag_amp * mag_amp + 0.0157 * mag_amp * mag_amp - 0.0209 * mag_amp + 1.0706);
                }
                if(mag_test[4] * vpp < 0.021){
                    mag_test[4] = mag_test[4] * (-0.0018 * mag_amp * mag_amp * mag_amp + 0.0228 * mag_amp * mag_amp - 0.0194 * mag_amp + 1.0756);
                }
            }
            else{
                if(mag_test[1] * vpp < 0.021){
                    mag_test[1] = mag_test[1] * (mag_amp * mag_amp * mag_amp * -0.0000007 + 0.000137 * mag_amp * mag_amp - 0.006 * mag_amp + 1.1657);
                }
                if(mag_test[2] * vpp < 0.021){
                    mag_test[2] = mag_test[2] * (mag_amp * mag_amp * mag_amp * 0.000000146- 0.000076 * mag_amp * mag_amp + 0.00854 * mag_amp + 0.981);
                }
                if(mag_test[3] * vpp < 0.021){
                    mag_test[3] = mag_test[3] * (mag_amp * mag_amp * mag_amp * 0.00000052- 0.000136 * mag_amp * mag_amp + 0.0099 * mag_amp + 1.07);
                }
                if(mag_test[4] * vpp < 0.021){
                    mag_test[4] = mag_test[4] * (mag_amp * mag_amp * mag_amp * 0.0000024 - 0.00049 * mag_amp * mag_amp + 0.0291 * mag_amp + 0.876);
                }

            }

            THD = sqrt(mag_test[1] * mag_test[1] + mag_test[2] * mag_test[2] + mag_test[3] * mag_test[3] + mag_test[4] * mag_test[4]) * 100;

            if(THD <= 55){
                f_out = (f_out * (10 - circle_count) + f_in) / (11 - circle_count);
                THD_out = (THD_out * (10 - circle_count) + THD) / (11 - circle_count);
                for(i = 0; i < 5; i++){
                    mag_out[i] = (mag_out[i] * (10 - circle_count) + mag_test[i]) / (11 - circle_count);
                }
                circle_count--;
            }

            if(circle_count == 0){
                for(i = 0; i < 10; i++){
                    /* Trigger the ADC conversion. */
                    MAP_ADCProcessorTrigger(ADC1_BASE, 3);

                    /* Wait for conversion to be completed. */
                    while(!MAP_ADCIntStatus(ADC1_BASE, 3, false))
                    {
                    }

                    /* Clear the ADC interrupt flag. */
                    MAP_ADCIntClear(ADC1_BASE, 3);

                    /* Read ADC Value. */
                    MAP_ADCSequenceDataGet(ADC1_BASE, 3, &getDCVoltage);

                    v_dc = (v_dc * i + getDCVoltage) / (i + 1);
                }

                v_dc = v_dc * 3.3 / 4096.0 * 1.02;
//                for(i = 1; i < 5; i++){
//                    mag_out[i] = mag_out[i] / mag_out[0];
//                }

                sig_max = -8192;
                sig_min = 8192;
                for(i = 0; i < 128; i++){
                    sig_out[i] = sin(6.283185306 * i / 128.0) + mag_out[1] * sin(6.283185306 * i / 64.0) + mag_out[2] * sin(6.283185306 * i  * 3 / 128.0) + mag_out[3] * sin(6.283185306 * i / 32.0) + mag_out[4] * sin(6.283185306 * i * 5/ 128.0);
                    sig_out[i] = sig_out[i] * 4096;
                    if(sig_out[i] > sig_max) sig_max = sig_out[i];
                    if(sig_out[i] < sig_min) sig_min = sig_out[i];
                }
                sig_pp = sig_max - sig_min;

                UARTprintf("%d%d%d:%d%d%d:%d%d%d:%d%d%d", (int32_t)(mag_out[1] * 10) % 10, (int32_t)(mag_out[1] * 100) % 10, (int32_t)(mag_out[1] * 1000) % 10,
                           (int32_t)(mag_out[2] * 10) % 10, (int32_t)(mag_out[2] * 100) % 10, (int32_t)(mag_out[2] * 1000) % 10,
                           (int32_t)(mag_out[3] * 10) % 10, (int32_t)(mag_out[3] * 100) % 10, (int32_t)(mag_out[3] * 1000) % 10,
                           (int32_t)(mag_out[4] * 10) % 10, (int32_t)(mag_out[4] * 100) % 10, (int32_t)(mag_out[4] * 1000) % 10);
                if(THD_out >= 10) UARTprintf(":%d%d.%d%d%%", (int32_t)(THD_out / 10), (int32_t)THD_out % 10, (int32_t)(THD_out * 10) % 10, (int32_t)(THD_out * 100) % 10);
                else UARTprintf(":%d.%d%d", (int32_t)THD_out % 10, (int32_t)(THD_out * 10) % 10, (int32_t)(THD_out * 100) % 10);

                for(i = 0; i < 128; i++){
                    v_out_temp = sig_out[i] * 3.0 / 8192.0 + 1.5;
                    UARTprintf(":%d%d%d", (int32_t)v_out_temp,  (int32_t)(v_out_temp * 10) % 10, (int32_t)(v_out_temp * 100) % 10);
                }

                UARTprintf("\r\n");

                UARTStdioConfig(7, 38400, systemClock); //串口屏开始
                UARTprintf("%c%c%c", 0xEE, 0xB1, 0x12);
                MAP_UARTCharPut(UART7_BASE, 0x00);
                MAP_UARTCharPut(UART7_BASE, 0x00);
                MAP_UARTCharPut(UART7_BASE, 0x00);
                MAP_UARTCharPut(UART7_BASE, 0x0A);
                UARTprintf("%c%c%d.%d%d%d", 0x00, 0x05, 1, 0, 0, 0); //基频为1
                UARTprintf("%c%c%c%c%d.%d%d%d", 0x00, 0x0B, 0x00, 0x05, 0, (int32_t)(mag_out[1] * 10) % 10, (int32_t)(mag_out[1] * 100) % 10, (int32_t)(mag_out[1] * 1000) % 10);
                UARTprintf("%c%c%c%c%d.%d%d%d", 0x00, 0x0C, 0x00, 0x05, 0, (int32_t)(mag_out[2] * 10) % 10, (int32_t)(mag_out[2] * 100) % 10, (int32_t)(mag_out[2] * 1000) % 10);
                UARTprintf("%c%c%c%c%d.%d%d%d", 0x00, 0x0D, 0x00, 0x05, 0, (int32_t)(mag_out[3] * 10) % 10, (int32_t)(mag_out[3] * 100) % 10, (int32_t)(mag_out[3] * 1000) % 10);
                UARTprintf("%c%c%c%c%d.%d%d%d", 0x00, 0x0E, 0x00, 0x05, 0, (int32_t)(mag_out[4] * 10) % 10, (int32_t)(mag_out[4] * 100) % 10, (int32_t)(mag_out[4] * 1000) % 10);
                if(f_in  / 100000.0 >= 1.0) UARTprintf("%c%c%c%c%d%d%d.%dkH%c", 0x00, 0x0F, 0x00, 0x08, (int32_t)(f_out / 100000), (int32_t)(f_out) % 100000 / 10000, (int32_t)(int32_t)(f_out) % 10000 / 1000, (int32_t)(f_out) % 1000 / 100, 'z');
                else if(f_in / 10000.0 >= 1.0) UARTprintf("%c%c%c%c%d%d.%dkH%c", 0x00, 0x0F, 0x00, 0x07, (int32_t)(f_out / 10000), (int32_t)(int32_t)(f_out) % 10000 / 1000, (int32_t)(f_out) % 1000 / 100, 'z');
                else UARTprintf("%c%c%c%c%d.%dkH%c", 0x00, 0x0F, 0x00, 0x06, (int32_t)(f_out / 1000), (int32_t)(f_out) % 1000 / 100, 'z');

                vpp = vpp * 1000 * 1.03/ AMP;
                if(vpp >= 100) UARTprintf("%c%c%c%c%dmV", 0x00, 0x10, 0x00, 0x05, (int32_t)(vpp));
                else if(vpp >= 10) UARTprintf("%c%c%c%c%dmV", 0x00, 0x10, 0x00, 0x04, (int32_t)(vpp));
                else UARTprintf("%c%c%c%c%dmV", 0x00, 0x10, 0x00, 0x03, (int32_t)(vpp));

                UARTprintf("%c%c%c%c%d.%d%dV", 0x00, 0x11, 0x00, 0x05, (int32_t)v_dc % 10, (int32_t)(v_dc * 10) % 10, (int32_t)(v_dc * 100) % 10);

                if(THD_out >= 10) UARTprintf("%c%c%c%c%d%d.%d%d", 0x00, 0x14, 0x00, 0x06, (int32_t)(THD_out / 10), (int32_t)THD_out % 10, (int32_t)(THD_out * 10) % 10, (int32_t)(THD_out * 100) % 10);
                else UARTprintf("%c%c%c%c%d.%d%d", 0x00, 0x14, 0x00, 0x05, (int32_t)THD_out % 10, (int32_t)(THD_out * 10) % 10, (int32_t)(THD_out * 100) % 10);
                UARTprintf("%c", '%');


                UARTprintf("%c%c%c%c", 0xFF, 0xFC, 0xFF, 0xFF);



                UARTprintf("%c%c%c%c%c%c%c", 0xEE, 0xB1, 0x33, 0x00, 0x00, 0x00, 0x09); //清空
                UARTprintf("%c", 0x00);
                UARTprintf("%c%c%c%c", 0xFF, 0xFC, 0xFF, 0xFF);

                UARTprintf("%c%c%c%c%c%c%c", 0xEE, 0xB1, 0x34, 0x00, 0x00, 0x00, 0x09); //配置缩放
                UARTprintf("%c%c%c%c", 0x00, 0x00, (char)((32000.0 / 129) / 256), (char)((uint32_t)(32000.0 / 129) % 256));
                UARTprintf("%c%c%c%c", 0x00, 0x00, 0x00, 0x79);
                UARTprintf("%c%c%c%c", 0xFF, 0xFC, 0xFF, 0xFF);

                UARTprintf("%c%c%c%c%c%c%c%c", 0xEE, 0xB1, 0x35, 0x00, 0x00, 0x00, 0x09, 0x00);
                UARTprintf("%c%c", 0x00, 0x82);

                vv_temp = vpp / 1.11 / 600.0 / sig_pp * 8192.0;

                for(i = 0; i < 128; i++){
                    v_out_temp = sig_out[i]  * 255 * vv_temp / 8192.0 + 128;
                    if(v_out_temp > 255) v_out_temp = 255;
                    if(v_out_temp < 0) v_out_temp = 0;
                    UARTprintf("%c", (char)v_out_temp);
                }

                for(i = 0; i < 2; i++){
                    v_out_temp = sig_out[i]  * 255 * vv_temp / 8192.0 + 128;
                    if(v_out_temp > 255) v_out_temp = 255;
                    if(v_out_temp < 0) v_out_temp = 0;
                    UARTprintf("%c", (char)v_out_temp);
                }


                UARTprintf("%c%c%c%c", 0xFF, 0xFC, 0xFF, 0xFF);

                UARTStdioConfig(2, 9600, systemClock); //串口屏结束

                THD_out = 0;
                mag_out[0] = 0;
                mag_out[1] = 0;
                mag_out[2] = 0;
                mag_out[3] = 0;
                mag_out[4] = 0;
                f_out = 0;
                v_dc = 0;
            }
        }
    }
}
