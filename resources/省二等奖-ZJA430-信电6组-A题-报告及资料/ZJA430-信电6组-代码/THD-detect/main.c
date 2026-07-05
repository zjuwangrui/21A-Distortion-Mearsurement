#define _MAIN_C_
#include "main.h"

void do_sdwa_update(void)
{
    uint8_t data[PBUFFERSIZE * 2] = {0};
    SDWa_InstrInfo a;
    int i, j = 0;
    u16 tmp;

    for (i = 0; i < PBUFFERSIZE; i++)
    {
        tmp = pBuffer[i];
        data[j++] = tmp >> 8;
        data[j++] = tmp & 0xff;
    }

    a.instr = CURV;
    a.CH_Mode = 1 << 0;
    a.fhead = 0xa55a;
    a.data_len = PBUFFERSIZE;
    a.data_ptr = data;
    SDWa_SendCurve(a);

    for (i =1;i<5;i++){
        a.instr = WVAR;
        a.start_addr = i;
        a.data_len = 1;
        tmp = Uo[i]*1000/Uo[0];
        data[0]=tmp >> 8;
        data[1]=tmp & 0xff;
        a.data_ptr = data;
        SDWa_Build(a);
        SDWa_Send();
    }

    a.start_addr = 5;
    tmp = THDx*10000;
    data[0]= tmp >> 8;
    data[1]=tmp & 0xff;
    a.data_ptr = data;
    SDWa_Build(a);
    SDWa_Send();

    u32 tmp1;
    a.start_addr = 6;
    tmp1 = BaseFreqDisp;
    a.data_len = 2;
    data[0]= tmp1 >> 24;
    data[1]=(tmp1>>16) & 0xff;
    data[2]=(tmp1>>8) & 0xff;
    data[3]=tmp1 & 0xff;
    a.data_ptr = data;
    SDWa_Build(a);
    SDWa_Send();
}

void do_tcp_update(void)
{
    int len=-1;
    char msg[4096];
    uint16_t uamp[4];
    int i;
    for(i=1;i<5;i++)
        uamp[i-1]=Uo[i]*1000/Uo[0];
    len = sprintf(msg,"BaseFreq=%u,THD=%u,%u,%u,%u,%u:",BaseFreq,(uint32_t)(THDx*10000),
                    uamp[0],uamp[1],uamp[2],uamp[3]);
    tcp_send(0,msg,len);
//    UARTprintf("--TCP--Send %d bytes\r\n", len);

//    for(i=0;i<PBUFFERSIZE;i++)
//    {
//        len = sprintf(msg, "%u,",pBuffer[i-1]);
//        tcp_send(0,msg,len);
//    }
//    UARTprintf("--TCP--Send %d bytes\r\n", len);
}

void draw_wave_buf(void)
{
    int i;
    float base;
    float composite;
    float sum=0;
    for(i=0;i<5;i++)
    {
        sum+=Uo[i];
    }
    if(sum<1e-5)
        return;
    for(i=0;i<PBUFFERSIZE;i++)
    {
        base = 2*PI/PBUFFERSIZE*i;
        composite = Uo[0]*sin(base)+  Uo[1]*sin(2*base)+
                    Uo[2]*sin(3*base)+Uo[3]*sin(4*base)+
                    Uo[4]*sin(5*base);
        pBuffer[i] = (composite+sum)/(2*sum)*4095;
    }
}

void update_wave_buf(void)
{
    int i;
    uint32_t save_SampleFreq = SampleFreq;
    uint32_t plimit=PBUFFERSIZE;
    SampleFreq = (PBUFFERSIZE/1)*BaseFreq;
    if(SampleFreq > 1e6)
    {
        SampleFreq = 1e6;
        plimit = SampleFreq/BaseFreq;
    }
//    UARTprintf("F_wavesample=%u, plimit=%u\r\n",SampleFreq, plimit);

    ADC_Sample();
    SampleFreq = save_SampleFreq;

    if(plimit==PBUFFERSIZE)
    {
//        UARTprintf("draw full\r\n");
        for(i=0;i<PBUFFERSIZE;i++)
            pBuffer[i]=adcBuffer[i+10];
    }
    else
    {
//        UARTprintf("draw interp\r\n");
        // 线性插值
        float delta = 1.0*plimit/PBUFFERSIZE;
        uint16_t a,b;
        for(i=0;i<PBUFFERSIZE;i++)
        {
            a = adcBuffer[(uint16_t)(i*delta)+10];
            b = adcBuffer[(uint16_t)(i*delta)+11];
            pBuffer[i] = a+(b-a)*(i*delta - (uint16_t)(i*delta));
        }
//        pBuffer[PBUFFERSIZE-1] = adcBuffer[plimit-1];
    }
    // auto scale
    uint16_t pmax=pBuffer[0];
    uint16_t pmin=pBuffer[0];
    double scale;
    for(i=0;i<PBUFFERSIZE;i++)
        if(pmax<pBuffer[i])
            pmax = pBuffer[i];
        else if(pmin>pBuffer[i])
            pmin=pBuffer[i];
    scale = pmax-pmin;
    if((scale)>1e-4)
        for(i=0;i<PBUFFERSIZE;i++)
            pBuffer[i]=(pBuffer[i]-pmin)*(4095)/scale;

    // debug
//    for(i=0;i<PBUFFERSIZE;i++)
//        UARTprintf("%d,\r\n",pBuffer[i]);
//    UARTprintf("\r\n");
}

void board_init(void)
{
    // variable init
    sdwa_send_flag = 0;
    sdwa_recv_flag = 0;
    systemClock = MAP_SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN |
                                          SYSCTL_USE_PLL | SYSCTL_CFG_VCO_480),
                                          120000000);
    // enable interrupt
    MAP_IntMasterEnable();
    // device init
    SDWa_UartInit();
    esp8266_init();

}

void compute_base_freq(void)
{
    uint32_t ii, base_flag;
    SampleFreq = 1e6;
    base_flag = 0;
    while(1)
    {
        ADC_Sample();

        /* First convert the sampled data to floating point format as the RMS
         * and DC average is being computed */
        for(ii=0;ii<NPT;ii++)
        {
            fft_in[ii] = 3.3*adcBuffer[ii]/4095;
//            UARTprintf("%d,\r\n", (uint32_t)fft_in[ii]*1000);
//            UARTprintf("%d,", adcBuffer[ii]);
        }
        //
        FFT(fft_out, fft_in, NPT);
        Get_Mag(fft_in, fft_out, NPT);

        for(ii = 0; ii<NPT/2; ii++)
        {
            if(!ii)
                fft_in[ii] = fft_in[ii]/NPT;
            else
            {
                fft_in[ii] = fft_in[ii]/NPT*2;
            }
        }

        Find_Fund(fft_in, NPT, &base_freq_index);
        BaseFreq = base_freq_index*SampleFreq/NPT;
        if(base_flag)  break;

        if(BaseFreq < 1e6/50 )
        {
            SampleFreq = 50*BaseFreq;
            base_flag++;
        }
        else if(BaseFreq < 1e6/20 )
        {
            SampleFreq = 20*BaseFreq;
            base_flag++;
        }
        else if(BaseFreq < 1e6/10 )
        {
            SampleFreq = 10*BaseFreq;
            base_flag++;
        }
        else
            break;
    }
//    UARTprintf("\r\nFs=%d,BaseFreq=%d\r\n",SampleFreq,  BaseFreq);
    BaseFreqDisp = BaseFreq;
    BaseFreq = (uint32_t)((float)BaseFreq/1000.0 + 0.5)*1000;
}

float compute_THD_goertzel(void)
{
    struct goertzel_state g;
    float sum=0;
    float thd = 0.0;
    int i,s;
    uint16_t num_periods = BaseFreq * NPT / SampleFreq;
    if(num_periods>10)
        num_periods = 10;
    uint16_t npoints = num_periods*SampleFreq/BaseFreq;
//    UARTprintf("--G--npoints = %u\r\n",npoints);
    for(i=0;i<5;i++)
    {
        goertzel_reset(&g);
        goertzel_init(&g, SampleFreq, BaseFreq*(i+1), npoints);
        for(s = 0; s < npoints; s++)
        {
            goertzel_process_sample(&g, (goertzel_storage_type)(adcBuffer[s]*3300.0/4095.0));
        }
        Uo[i] = goertzel_get_magnitude(&g)/npoints*2;
        if(i!=0)sum += Uo[i]*Uo[i];
//        UARTprintf("--G--%d Hz -> %d uV\r\n", BaseFreq*(i+1), (int)(Uo[i]*1000));
    }
    if(Uo[0]>1e-4)
        thd = sqrt(sum)/Uo[0];
//    UARTprintf("--G--thd = %u/1000\r\n",(uint32_t)(thd*1000));

//    for(s = 0; s < npoints; s++)
//    {
//        UARTprintf("AAAA%u,\r\n",adcBuffer[s]);
//    }
    return thd;
}

float compute_THD_fft(void)
{
    int i;
    uint32_t idx[4];
    float sum = 0;
    float thd = 0.0;
    for(i=2;i<6;i++)
    {
        Center_Find(fft_in,base_freq_index*i,idx+i-2);
    }
    Uo[0]=fft_in[base_freq_index];
    for(i=1;i<5;i++)
    {
        Uo[i]=fft_in[idx[i-1]];
        sum += Uo[i]*Uo[i];
    }

    if(Uo[0]>1e-4)
        thd = sqrt(sum)/Uo[0];
//    UARTprintf("--FFT--THDx = %u/1000\r\n",(uint32_t)(THDx*1000));
    return thd;
}

void compute_THD(void)
{
    // 减去一个最大值和一个最小值后取平均
    int i;
    float min, max, tmp,sum;
    sum=0;
    min=100;
    max=0;
    for(i=0;i<NUM_THD_AVERAGE;i++)
    {
        ADC_Sample();
        tmp=compute_THD_goertzel();
        sum+=tmp;
        if(tmp>max)
            max=tmp;
        if(tmp<min)
            min=tmp;
        SysCtlDelay(systemClock / 2000);
    }
    sum = (sum - min - max)/(NUM_THD_AVERAGE - 2);
    THDx=sum;
//    UARTprintf("THDx=%u\r\n",(uint32_t)(THDx*1000));
}

void update_displays(void)
{
//    update_wave_buf();
    draw_wave_buf();
    do_sdwa_update();
    do_tcp_update();
}
uint32_t main_cnt = 0;
void main(void)
{

    board_init();

    while(1)
    {
        while(!flags.btnStart);
        flags.btnStart=0;
        if(flags.doCompute){
            compute_base_freq();
            compute_THD();
            flags.doCompute = 0;
            flags.doDisplay = 1;
        }
        if(flags.doDisplay)
        {
            update_displays();
            flags.doDisplay = 0;
        }
        main_cnt++;
//        SysCtlDelay(systemClock / 2);
    }
}
