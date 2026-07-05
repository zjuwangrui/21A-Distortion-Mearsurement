#ifndef __Goertzel_H
#define __Goertzel_H

#include <ti/devices/msp432e4/driverlib/driverlib.h>

#define GoertzelSize 1024

uint32_t Goertzel_Mag(double cos_fact, int8_t mode);

#endif
