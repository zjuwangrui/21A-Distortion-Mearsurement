//灏佽arm_fft锛屼娇鐢ㄩ渶姹�
#ifndef _FFT_H
#define _FFT_H
// #include "arm_common_tables.h"
#include "arm_const_structs.h"
#include "arm_math.h"

//#define  NPT   1024
//锟斤拷锟斤拷锟斤拷
#define HANNING(n) (0.5 - 0.5 * cos(2 * PI * n / (NPT - 1)))
#define HANCOEFF 2
//锟斤拷锟斤拷锟斤拷
#define HAIMING(n) (0.54 - 0.46 * cos(2 * PI * n / (NPT - 1)))
#define HAICOEFF 1.852
//锟斤拷锟斤拷锟斤拷锟斤拷锟斤拷
#define BLACKMAN(n) (0.42 - 0.5 * cos(2 * PI * (n - 1) / (NPT - 1)) + 0.008 * cos(4 * PI * (n - 1) / (NPT - 1)))
#define BLACKCOEFF 2.381

#define FLATTOPWIM(n)  (0.21558-0.41663*cos(2*PI*n/(NPT-1))+\
                        0.27726*cos(4*PI*n/(NPT-1))-\
                        0.08358*cos(6*PI*n/(NPT-1))+\
                        0.00695*cos(8*PI*n/(NPT-1)))
#define FLATCOEFF   1.0     //not sure


//p_out: complex vector, p_mag: magnitude , p_in : sampling value, samples : the number of sampling.
void FFT(float32_t*p_out, float32_t*p_in, uint32_t samples);
void Get_Mag(float32_t*p_out, float32_t*p_in, uint32_t samples);
void Find_Fund(float32_t*A, uint32_t samples, uint32_t*index);
void Center_Find(float32_t*A, uint32_t center, uint32_t*index);

//鍙�夊嚱鏁�
// void Get_Phase(float32_t*p_out, float32_t*p_in, uint16_t samples);
// void Get_Fund_Index(float32_t*A, uint16_t samples,  uint16_t*rt_index);
void Get_Index_Base_Freq(float32_t*mag, uint32_t*index, uint32_t samples);

//get one periodic data
void Get_Period_Data(uint32_t*per_data, uint32_t*num, uint32_t*adcBuf, 
                    uint32_t samples, uint32_t sample_fs, uint32_t base_freq);
#endif
