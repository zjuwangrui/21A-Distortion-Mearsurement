/**
 * @file bsp_sdwa.c
 * @author zch (hzyhzch@qq.com)
 * @brief common struct and enum that are safe to include
 * @version 0.1
 * @date 2021/07/12
 * it's all about TI compiler...
 *
 * @copyright Copyright (c) 2021
 *
 */
#ifndef __SDWA_COMMON_H_
#define __SDWA_COMMON_H_
/*   ---------------------------------------------------------------------------------
    |2B  帧头|1B长度|1B指令|寄存器1B,变量存储器2B|寄存器1B,变量存储器2B| NB or 2NB |    2B   |
    |寻址用  |      |     |    起始地址        |  [数据长度N]       | [数据内容] |  [CRC]  |
    ----------------------------------------------------------------------------------

    NOTE:
    CRC校验不包括指令帧头和指令长度，采用ANSI-16（X16+X15+X2+1）格式。
*/

#include <ti/devices/msp432e4/driverlib/driverlib.h>
#include "uartstdio.h"
#include "printf.h"

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;

typedef struct
{
    uint8_t instr;       // 指令
    uint16_t fhead;      // 帧头
    uint16_t start_addr; // 起始地址
    uint8_t data_len;    // 数据长度
    uint8_t *data_ptr;   // 可选数据
    uint8_t CH_Mode;     // 曲线通道
} SDWa_InstrInfo;

#if DEBUG
#define assert(expr)                            \
    do                                          \
    {                                           \
        if (!(expr))                            \
        {                                       \
            UARTprintf("Assertion failed @"__FILE__ \
                   ":%d \n",                    \
                   __LINE__);                   \
            return;                             \
        }                                       \
    } while (0)
#else
#define assert(expr) \
    do               \
    {                \
        if (!(expr)) \
            return;  \
    } while (0)
#endif

enum SDWa_instr
{
    WREG = 0x80,
    RREG = 0x81,
    WVAR = 0x82,
    RVAR = 0x83,
    CURV = 0x84,
    EXT = 0x85
};


#define SDWA_UART              UART0_BASE
#define SDWA_UART_INT          INT_UART0
#define SDWA_PERI_UART         SYSCTL_PERIPH_UART0
#define SDWA_GPIO              GPIO_PORTA_BASE
#define SDWA_GPIO_PINS         (GPIO_PIN_0 | GPIO_PIN_1)
#define SDWA_PERI_GPIO         SYSCTL_PERIPH_GPIOA
#define SDWA_GPIO_PIN_CONF_RX  GPIO_PA0_U0RX
#define SDWA_GPIO_PIN_CONF_TX  GPIO_PA1_U0TX
#define SDWA_IRQHandler        UART0_IRQHandler

#endif /* __SDWA_COMMON_H_ */

