/*
 * main_common.h
 *
 *  Created on: 2021Äê11ÔÂ6ÈÕ
 *      Author: DoctorX
 */

#ifndef MAIN_COMMON_H_
#define MAIN_COMMON_H_

#define NPT 2048
#define PBUFFERSIZE 82
#define NUM_THD_AVERAGE 5
#if NUM_THD_AVERAGE<2
#error "NUM_THD_AVERAGE at least 2"
#endif

typedef struct
{
    int doCompute;
    int doDisplay;
    int btnStart;

} CTRL_FLAGS;

#endif /* MAIN_COMMON_H_ */
