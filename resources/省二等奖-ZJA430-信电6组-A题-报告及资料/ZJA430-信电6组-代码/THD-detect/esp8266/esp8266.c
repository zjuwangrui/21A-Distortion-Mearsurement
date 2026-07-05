#include <ti/devices/msp432e4/driverlib/driverlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "printf.h"
#include "esp8266.h"

static uint8_t receive_buffer[128];         //回显信息缓存
static uint8_t receive_len;                 //回显信息长度
//static uint8_t tcp_ok;

extern uint8_t tcp_link;            //手机连接成功标志
extern uint8_t tcp_start[5];                   //检测启动标志
extern uint32_t PRINTF_UART_BASE;

extern uint32_t systemClock;

void _putchar(char character)
{
    MAP_UARTCharPut(PRINTF_UART_BASE, character);
}

static void Delay_us(uint32_t time)    //延时，单位为us
{
    uint32_t i;
    for ( ; time>0; time--)
    {
        for( i=20; i>0; i--);
    }
}

void UART3_IRQHandler(void)
{
    uint32_t getIntStatus;
    int32_t get_char;
    uint8_t i;

    /* Read the interrupt status from the UART */
    getIntStatus = MAP_UARTIntStatus(UART3_BASE, true);

    if((getIntStatus & UART_INT_RX) == UART_INT_RX)
    {
        get_char = MAP_UARTCharGet(UART3_BASE);
        receive_buffer[receive_len++] = get_char;
        if((get_char == '\n'))
        {
            receive_buffer[receive_len - 1] = '\0';
            PRINTF_UART_BASE = UART0_BASE;
//            printf("received:%s\n",receive_buffer);
            if(strcmp((char*)receive_buffer,"Link\r") == 0)
                tcp_link = 1;
//            else if(tcp_link)
//            {
//                tcp_link=0;
//                if(strcmp((char*)receive_buffer,"+IPD,0,10:THD-detect\r") == 0)
//                    tcp_start[0] = 1;
//                else if(strcmp((char*)receive_buffer,"+IPD,1,10:THD-detect\r") == 0)
//                    tcp_start[1] = 1;
//                else if(strcmp((char*)receive_buffer,"+IPD,2,10:THD-detect\r") == 0)
//                    tcp_start[2] = 1;
//                else if(strcmp((char*)receive_buffer,"+IPD,3,10:THD-detect\r") == 0)
//                    tcp_start[3] = 1;
//                else if(strcmp((char*)receive_buffer,"+IPD,4,10:THD-detect\r") == 0)
//                    tcp_start[4] = 1;
//            }
//            else if(strcmp((char*)receive_buffer,"OK\r") == 0)
//                tcp_ok = 1;

            for(i = 0; i < 128; i++)
                receive_buffer[i]=0;
            PRINTF_UART_BASE = UART3_BASE;
            receive_len = 0;
        }
    }
}

void ConfigUART3(void)
{
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    while(!(MAP_SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOA)));
    MAP_GPIOPinConfigure(GPIO_PA4_U3RX);
    MAP_GPIOPinConfigure(GPIO_PA5_U3TX);
    MAP_GPIOPinTypeUART(GPIO_PORTA_BASE, (GPIO_PIN_4 | GPIO_PIN_5));
    /* Enable the clock to the UART-3 and wait for it to be ready */
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART3);
    while(!(MAP_SysCtlPeripheralReady(SYSCTL_PERIPH_UART3)));
    /* Configure UART for 9600 bps, 8-N-1 format. Enable the DMA Request
     * generation and interrupt on DMARX Done. Also make sure that the FIFO is
     * disabled. */
    MAP_UARTConfigSetExpClk(UART3_BASE, systemClock, 9600,
                            (UART_CONFIG_WLEN_8 | UART_CONFIG_PAR_NONE |
                             UART_CONFIG_STOP_ONE));
    MAP_UARTIntEnable(UART3_BASE, UART_INT_RX | UART_INT_RT);
    MAP_UARTEnable(UART3_BASE);
    MAP_UARTFIFODisable(UART3_BASE);
    MAP_IntEnable(INT_UART3);
}

void esp8266_init(void)     //esp8266模块启动tcp服务器
{
    receive_len = 0;
    ConfigUART3();
    PRINTF_UART_BASE = UART3_BASE;
//    printf("AT+RST\r\n");
//    while(!tcp_ok);
//    tcp_ok=0;
//    Delay_us(1e5);
    PRINTF_UART_BASE = UART3_BASE;
    printf("AT+CIPMUX=1\r\n");
//    while(!tcp_ok);
//    tcp_ok=0;
    Delay_us(1e5);
    PRINTF_UART_BASE = UART3_BASE;
    printf("AT+CIPSERVER=1,8080\r\n");
//    while(!tcp_ok);
//    tcp_ok=0;
    Delay_us(1e5);
    PRINTF_UART_BASE = UART0_BASE;
}

void tcp_send(uint8_t id, uint8_t* pstr, uint32_t len)  //tcp信息发送
{
    if(id>4)
        return;
    PRINTF_UART_BASE = UART3_BASE;
    printf("AT+CIPSEND=%d,%d\r\n", id,len);
    printf("%s\r\n", pstr);
    PRINTF_UART_BASE = UART0_BASE;
}
