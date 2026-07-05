/*
 * adc.c
 *
 *  Created on: 2021Äê11ÔÂ5ÈÕ
 *      Author: DoctorX
 */
#include "adc.h"

extern uint32_t systemClock,SampleFreq;
extern uint16_t adcBuffer[NPT];
extern bool adcDone;

volatile bool bufA_done = false;
volatile bool bufB_done = false;

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



uint32_t cnt1=0,cnt2=0;
uint32_t adc_int_cnt = 0;
void ADC0SS3_IRQHandler(void)
{
    uint32_t getIntStatus;
    uint32_t getDMAStatus;
    adc_int_cnt++;

    getIntStatus = MAP_ADCIntStatusEx(ADC0_BASE, true);
    MAP_ADCIntClearEx(ADC0_BASE, getIntStatus);

    getDMAStatus = MAP_uDMAChannelModeGet(UDMA_CH17_ADC0_3 | UDMA_PRI_SELECT);
    if(getDMAStatus == UDMA_MODE_STOP)
    {
        cnt1++;
        bufA_done = true;
    }

    getDMAStatus = MAP_uDMAChannelModeGet(UDMA_CH17_ADC0_3 | UDMA_ALT_SELECT);
    if(getDMAStatus == UDMA_MODE_STOP)
    {
        cnt2++;
        bufB_done = true;
    }
    if(bufA_done&&bufB_done){
//        MAP_IntMasterDisable();

//        MAP_TimerControlTrigger(TIMER0_BASE, TIMER_A, false);
        //MAP_ADCSequenceDisable(ADC0_BASE, 3);
//        MAP_uDMADisable();
        MAP_IntDisable(INT_ADC0SS3);
        //MAP_TimerDisable(TIMER0_BASE, TIMER_A);
        //MAP_uDMAChannelDisable(UDMA_CH17_ADC0_3);
        MAP_ADCIntClear(ADC0_BASE, 3);
    }
}

static void ConfigureADC0(void)
{
    // ADC0 init
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
    while(!(MAP_SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOE)));
    MAP_GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_3);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);
    while(!(MAP_SysCtlPeripheralReady(SYSCTL_PERIPH_ADC0)));

//    ADCClockConfigSet(ADC0_BASE,ADC_CLOCK_SRC_PLL | ADC_CLOCK_RATE_FULL, 15);

    MAP_ADCSequenceStepConfigure(ADC0_BASE, 3, 0, ADC_CTL_CH0 | ADC_CTL_IE | ADC_CTL_END);
    MAP_ADCSequenceConfigure(ADC0_BASE, 3, ADC_TRIGGER_TIMER, 2);
    MAP_ADCIntClearEx(ADC0_BASE, ADC_INT_DMA_SS3);
    MAP_ADCIntEnableEx(ADC0_BASE, ADC_INT_DMA_SS3);
    MAP_ADCSequenceDMAEnable(ADC0_BASE, 3);
    MAP_ADCSequenceEnable(ADC0_BASE, 3);
    MAP_IntEnable(INT_ADC0SS3);
}

static void ConfigureUDMA(void)
{
    // uDMA init
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_UDMA);
    while(!(SysCtlPeripheralReady(SYSCTL_PERIPH_UDMA)));
    MAP_uDMAEnable();

    /* Point at the control table to use for channel control structures. */
    MAP_uDMAControlBaseSet(pui8ControlTable);

    /* Map the ADC0 Sequencer 2 DMA channel */
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

    /* Set up the transfer parameters for the ADC0 Sequencer 2 primary control
     * structure. The mode is Basic mode so it will run to completion. */
    MAP_uDMAChannelTransferSet(UDMA_CH17_ADC0_3 | UDMA_PRI_SELECT,
                               UDMA_MODE_PINGPONG,
                               (void *)&ADC0->SSFIFO3, (void *)adcBuffer,
                               NPT/2);
    MAP_uDMAChannelControlSet(UDMA_CH17_ADC0_3 | UDMA_ALT_SELECT,
                             UDMA_SIZE_16 | UDMA_SRC_INC_NONE | UDMA_DST_INC_16 |
                             UDMA_ARB_1);

   /* Set up the transfer parameters for the ADC0 Sequencer 2 primary control
    * structure. The mode is Basic mode so it will run to completion. */
   MAP_uDMAChannelTransferSet(UDMA_CH17_ADC0_3 | UDMA_ALT_SELECT,
                              UDMA_MODE_PINGPONG,
                              (void *)&ADC0->SSFIFO3, (void *)(adcBuffer+NPT/2),
                              NPT/2);

    /* The uDMA ADC0 Sequencer 2 channel is primed to start a transfer. As
     * soon as the channel is enabled and the Timer will issue an ADC trigger,
     * the ADC will perform the conversion and send a DMA Request. The data
     * transfers will begin. */
    MAP_uDMAChannelEnable(UDMA_CH17_ADC0_3);
}

static void EnableTimerA(void)
{
    /* Enable Timer-0 clock and configure the timer in periodic mode with
     * a frequency of 1 KHz. Enable the ADC trigger generation from the
     * timer-0. */
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
    while(!(MAP_SysCtlPeripheralReady(SYSCTL_PERIPH_TIMER0)));

    MAP_TimerConfigure(TIMER0_BASE, TIMER_CFG_A_PERIODIC);
    MAP_TimerLoadSet(TIMER0_BASE, TIMER_A, (systemClock/SampleFreq));
    MAP_TimerADCEventSet(TIMER0_BASE, TIMER_ADC_TIMEOUT_A);
    MAP_TimerControlTrigger(TIMER0_BASE, TIMER_A, true);
    MAP_TimerEnable(TIMER0_BASE, TIMER_A);
}
uint32_t adc_sample_cnt = 0;
void ADC_Sample(void)
{
    adcDone = false;
    MAP_IntMasterDisable();
    bufA_done = false;
    bufB_done = false;
    ConfigureADC0();
    ConfigureUDMA();
    EnableTimerA();
    MAP_IntMasterEnable();
    while(!(bufA_done && bufB_done));
    adc_sample_cnt++;
    adcDone = true;
}
