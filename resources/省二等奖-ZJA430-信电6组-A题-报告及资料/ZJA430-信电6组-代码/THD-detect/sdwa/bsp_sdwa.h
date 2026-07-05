/**
 * @file bsp_sdwa.h
 * @author zch (hzyhzch@qq.com)
 * @brief header for bsp_sdwa.c
 * @version 0.1
 * @date 2021/07/12 11:11:04
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#ifndef __SDWA_H_
#define __SDWA_H_

#include "bsp_sdwa_common.h"

void SDWa_Build(SDWa_InstrInfo instr);
void SDWa_Send(void);
void SDWa_SendCurve(SDWa_InstrInfo instr);
void SDWa_UartInit(void);

extern char *display_recv_state[];
extern int sdwa_send_flag;
extern int sdwa_recv_flag;
extern int sdwa_decode_state;
extern int sdwa_rx_cnt;
extern uint16_t recv_data[];
extern SDWa_InstrInfo sdwa_recv;

#endif /* __SDWA_H_ */
