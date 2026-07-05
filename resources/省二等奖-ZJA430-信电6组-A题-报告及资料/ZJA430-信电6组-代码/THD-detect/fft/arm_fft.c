#include "arm_fft.h"
// #define  WINDOW
// #define ARMMAX
void FFT(float32_t*p_out, float32_t*p_in, uint32_t samples)
{
    arm_rfft_fast_instance_f32 S;
    arm_rfft_fast_init_f32(&S, samples);
    arm_rfft_fast_f32(&S, p_in, p_out,0);
}

//samples/2 娴ｈ法鏁_mag閸欏倹鏆熼敍灞藉灟濮濄倕鍤遍弫棰佺瑝闂囷拷鐟曪拷
void Get_Mag(float32_t*p_out, float32_t*p_in, uint32_t samples)
{
    arm_cmplx_mag_f32(p_in, p_out, samples);
}

//閼奉亜鐣炬稊澶婂毐閺侊拷, 閸旂姳绗夐崥宀�娈戠粣妤�鍤遍弫鏉垮讲閼充粙娓剁憰浣界殶閺佺⒑j閻ㄥ嫬锟界》绱濇笟瀣洤濮瑰妲戠粣妞剧窗濮光剝鐓嬮梽鍕箮妫版垼姘ㄩ敍灞筋嚤閼凤拷0閿涳拷1婢跺嫯绁撮崐鐓庣唨閺堫剛娴夐崥锟�
void Find_Fund(float32_t*A, uint32_t samples, uint32_t*index)
{
#ifndef ARMMAX
    float32_t tmp;
    int _k, _j=2;//閺嶈宓佺�圭偤妾担璺ㄦ暏娣囶喗鏁�
    samples = samples/2;
    for(_k = 3, tmp = A[_j]; _k < samples; _k++)
    {
      if(A[_k] - tmp > 1e-6)
      {
        tmp = A[_k];
        _j = _k;
      }
    }
    *index = _j;
#else 
  float32_t tmp ;
  arm_max_f32(A, samples/2, &tmp, index);
#endif
}
void Center_Find(float32_t*A, uint32_t center, uint32_t*index)
{
    float32_t tmp;
    int _k, _j=center;
    for(_k = center-5, tmp = A[center]; _k < center+6; _k++)
    {
      if(A[_k] > tmp )
      {
        tmp = A[_k];
        _j = _k;
      }
    }
    *index = _j;
}

//閸欘垶锟藉鍤遍弫甯礉娑撳锭ind_Fund()閻╃鎮�
void Get_Index_Base_Freq(float32_t*mag, uint32_t*index, uint32_t samples)
{
  uint32_t i, N;
  float32_t tmp;
  *index = 5;
  i = *index +1;
  N = samples/2;
  tmp = mag[*index];
  while(i<N)
  {
    // bigger
    if(mag[i] - tmp > 1e6)
    {
      *index = i;
      tmp = mag[i];
    }
    i++;
  }
}



//get one periodic data
void Get_Period_Data(uint32_t*per_data, uint32_t*num, uint32_t*adcBuf, uint32_t samples, uint32_t sample_fs, uint32_t base_freq)
{
  uint32_t i;
  *num = sample_fs/base_freq;
  //easy way
  for(i = 0; i<*num; i++)
  {
    per_data[i] = adcBuf[i];
  }
}

//end
