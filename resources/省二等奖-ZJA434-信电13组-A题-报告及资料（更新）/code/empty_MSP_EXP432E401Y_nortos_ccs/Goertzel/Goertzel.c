#include "Goertzel.h"
#include "math.h"
#include "./window/inc/window.h"

// = (cos (2*PI*(Freq/SAMPLE_RATE))) * 256*128;
#define COS_4000Hz 32688
#define COS_8000Hz 32449
#define COS_12000Hz 32052
#define COS_16000Hz 31499
#define COS_20000Hz 30792

#define N 1024

extern int32_t *getADCBuffer(int32_t index);
extern double v_ave;


uint32_t Goertzel_Mag(double cos_fact, int8_t mode)
{
  long long  v0, v1, v2; 
  long long  pwr;
  long long p1, p2, p01;
  uint32_t  i;
  
  v1 = 0;
  v2 = 0;
  for(i = 0; i < N; i++){
    if(mode == 0){
        v0 = (*(getADCBuffer(i)) - v_ave) *  hanning_1024[i] + ((v1*cos_fact)*2) - v2;
    }
    else{
        v0 = (*(getADCBuffer(i)) - v_ave) + ((v1*cos_fact)*2) - v2;
    }
    v2 = v1;
    v1 = v0;
  }
  
  p1 = v1 * v1;
  p2 = v2 * v2;
  p01 = v1 * v2;
  pwr = sqrt(p1 - ((cos_fact * p01)*2) + p2);
  
  if(pwr < 0) return(0);
  return (pwr>>8);
}

