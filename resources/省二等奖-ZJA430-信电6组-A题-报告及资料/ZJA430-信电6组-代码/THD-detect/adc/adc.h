/*
 * adc.h
 *
 *  Created on: 2021Äê11ÔÂ5ÈÕ
 *      Author: DoctorX
 */
#ifndef _ADC_H_
#define _ADC_H_

#include <ti/devices/msp432e4/driverlib/driverlib.h>

/* Standard Includes */
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

/* Display Include via console */
#include "uartstdio.h"
#include "main_common.h"

void ADC_Sample(void);

#endif
