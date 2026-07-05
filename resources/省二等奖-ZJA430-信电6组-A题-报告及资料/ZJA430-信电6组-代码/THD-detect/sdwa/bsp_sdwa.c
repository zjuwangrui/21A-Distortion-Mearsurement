/**
 * @file bsp_sdwa.c
 * @author zch (hzyhzch@qq.com)
 * @brief bsp for SDWa serial screen of Viewtech
 * @version 0.1
 * @date 2021/07/12 11:09:37
 * port version on MSP432E4
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#include "bsp_sdwa_common.h"
#include "bsp_sdwa.h"
#include "main_common.h"

extern u32 systemClock;
extern CTRL_FLAGS flags;

static uint8_t recv_index = 0;
static uint8_t len = 0;
static uint8_t recv_buf[50] = {0};       //串口接收指令
static uint8_t sdwa_send_buf[3 + 255];

char *display_recv_state[] = {
    "idle",
    "write_reg",
    "read_reg",
    "write_var",
    "read_var",
    "0x84",
    "0x85"};
int sdwa_send_flag=0;
int sdwa_recv_flag=0;
int sdwa_decode_state = 0;
int sdwa_rx_cnt = 0;
uint16_t recv_data[50] = {0}; // 接收数据内容
SDWa_InstrInfo sdwa_recv;


static void UARTSend( uint8_t *pui8Buffer, uint32_t ui32Count)
{
    while(ui32Count--)
    {
        MAP_UARTCharPut(SDWA_UART, (unsigned char)*pui8Buffer++);
    }
}

// RX interrupt handler
void SDWA_IRQHandler(void)
{
   uint32_t ui32Status;
   ui32Status = MAP_UARTIntStatus(SDWA_UART, true);
   if ((ui32Status & UART_INT_RX) != UART_INT_RX)
       return;
   MAP_UARTIntClear(SDWA_UART, UART_INT_RX);

  uint8_t ucTemp;
  int i;
    //逐字节接收指令
    ucTemp = MAP_UARTCharGet(SDWA_UART);
    recv_buf[recv_index] = ucTemp;
//    MAP_UARTCharPut(SDWA_UART, ucTemp); //回显
    recv_index++;
    sdwa_rx_cnt++;
      if (recv_index == 3)
      {
        //计算指令长度
        len = recv_buf[2];
        len += 3;
      }
      if (recv_index == len)
      {
        sdwa_recv.fhead = recv_buf[0] << 8 | recv_buf[1];
        sdwa_recv.instr = recv_buf[3];

        recv_index = 0;
        switch (sdwa_recv.instr)
        {
        case 0x80: // 写寄存器
          sdwa_decode_state = 1;
          sdwa_recv.start_addr = recv_buf[4];
          break;
        case 0x81: // 读寄存器
          sdwa_decode_state = 2;
          sdwa_recv.start_addr = recv_buf[4];
          assert(len >= 6);
          sdwa_recv.data_len = recv_buf[5];
          assert(len == 6 + sdwa_recv.data_len);
          for (i = 0; i < sdwa_recv.data_len; i++)
            recv_data[i] = recv_buf[6 + i];
          sdwa_recv.data_ptr = recv_data;
          break;
        case 0x82: // 写变量存储器
          sdwa_decode_state = 3;
          sdwa_recv.start_addr = recv_buf[4] << 8 | recv_buf[5];
          break;
        case 0x83: // 读变量存储器
          sdwa_decode_state = 4;
          sdwa_recv.start_addr = recv_buf[4] << 8 | recv_buf[5];
          assert(len >= 7);
          sdwa_recv.data_len = recv_buf[6];
          assert(len == 7 + sdwa_recv.data_len * 2);
          for (i = 0; i < sdwa_recv.data_len; i++)
            recv_data[i] = recv_buf[7 + 2 * i] << 8 | recv_buf[8 + 2 * i]; // 2NB
          sdwa_recv.data_ptr = recv_data;
          break;
        case 0x84:
          sdwa_decode_state = 5;
          break;
        case 0x85:
          sdwa_decode_state = 6;
          break;
        default:
          sdwa_decode_state = 0;
          break;
        }
        if (sdwa_decode_state)
          sdwa_recv_flag = 1;
        if(sdwa_decode_state == 4)
        {
//            printf(">>>addr=%u, len=%u, data=0x%x\r\n",
//                   sdwa_recv.start_addr, sdwa_recv.data_len, recv_data[0]);
            if(sdwa_recv.start_addr==0 && sdwa_recv.data_len==1 && recv_data[0] == 0xffff)
            {
                flags.btnStart = 1;
                flags.doCompute = 1;
            }
        }
      }

}

void SDWa_UartInit(void)
{
    /* Enable the clock to GPIO port A and UART 0 */
    MAP_SysCtlPeripheralEnable(SDWA_PERI_GPIO);
    MAP_SysCtlPeripheralEnable(SDWA_PERI_UART);

    /* Configure the GPIO Port A for UART 0 */
    MAP_GPIOPinConfigure(SDWA_GPIO_PIN_CONF_RX);
    MAP_GPIOPinConfigure(SDWA_GPIO_PIN_CONF_TX);
    MAP_GPIOPinTypeUART(SDWA_GPIO,SDWA_GPIO_PINS );

    /* Configure the UART for 115200 bps 8-N-1 format */
    UARTStdioConfig(0, 115200, systemClock);
    MAP_UARTFIFODisable(SDWA_UART);
    MAP_UARTIntEnable(SDWA_UART, UART_INT_RX | UART_INT_RT);
    MAP_IntEnable(SDWA_UART_INT);
}

/**
 * @brief 根据instr结构体构造串口屏指令
 * 
 * @param instr 
 */
void SDWa_Build(SDWa_InstrInfo instr)
{
    uint8_t instr_len = 3; // fixed length
    u8 index;
    u8 i;

    sdwa_send_buf[0] = instr.fhead >> 8;
    sdwa_send_buf[1] = instr.fhead & 0x00ff;
    sdwa_send_buf[3] = instr.instr;
    index = 4;

    if (instr.data_len >= 250)
        return;
    switch (instr.instr)
    {
    case 0x80: // 写寄存器
        sdwa_send_buf[index++] = instr.start_addr;
        assert(instr.data_len > 0 && instr.data_ptr != 0);
        for (i = 0; i < instr.data_len; i++)
            sdwa_send_buf[index++] = instr.data_ptr[i];
        instr_len = index;
        break;
    case 0x81: // 读寄存器
        sdwa_send_buf[index++] = instr.start_addr;
        sdwa_send_buf[index++] = instr.data_len;
        instr_len = index;
        break;
    case 0x82: // 写变量存储器
        sdwa_send_buf[index++] = instr.start_addr >> 8;
        sdwa_send_buf[index++] = instr.start_addr & 0x00ff;
        assert(instr.data_len > 0 && instr.data_ptr != 0);
        for (i = 0; i < instr.data_len * 2; i++) // 2NB
            sdwa_send_buf[index++] = instr.data_ptr[i];
        instr_len = index;
        break;
    case 0x83: // 读变量存储器
        sdwa_send_buf[index++] = instr.start_addr >> 8;
        sdwa_send_buf[index++] = instr.start_addr & 0x00ff;
        sdwa_send_buf[index++] = instr.data_len;
        instr_len = index;
        break;
    case 0x84: // 写曲线缓冲区
        sdwa_send_buf[index++] = instr.CH_Mode;
        for (i = 0; i < instr.data_len * 2; i++) // 2NB
            sdwa_send_buf[index++] = instr.data_ptr[i];
        instr_len = index;
        break;
    case 0x85: // 扩展指令
        break;
    default:
        break;
    }
    sdwa_send_buf[2] = instr_len - 3;
}
/**
 * @brief 通过USART发送指令
 * 
 */
void SDWa_Send(void)
{
    uint16_t len = sdwa_send_buf[2] + 3;
//    MAP_UARTFIFOLevelSet(SDWA_UART, UART_FIFO_TX1_8, UART_FIFO_RX1_8);
//    UARTFIFOEnable(SDWA_UART);
    UARTSend(sdwa_send_buf, len);
//    UARTFIFODisable(SDWA_UART);
}

void SDWa_SendCurve(SDWa_InstrInfo instr)
{
    u16 buf_index, old_index, send_times, residual;
    u16 data_index;
    u8 i, j;
    const u8 fragment_size = 72;
    u32 delay;

    if (instr.instr != CURV || fragment_size > 250)
        return;

    sdwa_send_buf[0] = instr.fhead >> 8;
    sdwa_send_buf[1] = instr.fhead & 0x00ff;
    sdwa_send_buf[3] = instr.instr;
    buf_index = 4;
    sdwa_send_buf[buf_index++] = instr.CH_Mode;

    send_times = instr.data_len * 2 / fragment_size;
    residual = instr.data_len * 2 - send_times * fragment_size;
//     UARTprintf("%d=%d*%d+%d\r\n", instr.data_len * 2, send_times, fragment_size, residual);
    old_index = buf_index;
    data_index = 0;
    for (j = 0; j < send_times; j++)
    {
        for (i = 0; i < fragment_size; i++)
            sdwa_send_buf[buf_index++] = instr.data_ptr[data_index++];
        sdwa_send_buf[2] = (buf_index - 3);
//         UARTprintf("Send_length=%d\r\n", sdwa_send_buf[2]);
        SDWa_Send();
        buf_index = old_index;                      //restore previous index, which is 5
         for (delay = 0x1ffff; delay != 0; delay--) // delay if necessary
             ;
    }
    if (residual > 0)
    {
        for (i = 0; i < residual; i++)
            sdwa_send_buf[buf_index++] = instr.data_ptr[data_index++];
        sdwa_send_buf[2] = (buf_index - 3);

//         UARTprintf("Send_length=%d\r\n", sdwa_send_buf[2]);

        SDWa_Send();
    }
}
