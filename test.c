#include <stdint.h>
#include <stdio.h>

    typedef struct
    {
        int32_t res;
        uint8_t  temp;
    } SResToTemp;

    SResToTemp res2temp[] =
    {
        {140000l, 15},  // 0
        {115000l, 20},  // 1
        {99500l,  25},  // 2
        {75530l,  30},  // 3
        {46050l,  40},  // 4
        {31070l,  50},  // 5
        {20800l , 60},  // 6
        {14490l,  70},  // 7
        {10000l,  80},  // 8
        {7160l,   90},  // 9
        {5220l,   100}, // 10
        {3860l,   110}, // 11
    };

void calcTemp(int32_t res)
{
//    int32_t v    = (adc_val * 3300ul) / 1024ul;
//    int32_t res  = 100000ul * v / (3300ul - v);
    int32_t temp = 50;

    printf("in: %i\n", res);
    for (uint8_t i = 0; i < ((sizeof(res2temp) / sizeof(SResToTemp)) - 1); i++)
    {
        printf("try: %i. res = %i, res0 = %i, res1 = %i\n", i, res, res2temp[i].res, res2temp[i+1].res);
        if ((res2temp[i].res > res) && (res2temp[i+1].res <= res))
        {
            int32_t dR1 = res2temp[i+1].res  - res2temp[i].res;
            int32_t dT1 = res2temp[i+1].temp - res2temp[i].temp;
            int32_t dR2 = res - res2temp[i].res;
            int32_t dT2 = dT1 * dR2 / dR1;
            int32_t temp = res2temp[i].temp + dT2;

            printf("dR1 = %i, dT1 = %i, dR2 = %i, dT2 = %i, T = %i\n", dR1, dT1, dR2, dT2, temp);
            break;
        }
    }
}

void main()
{
    calcTemp(120000);
    calcTemp(133000);
    calcTemp(131000);
    calcTemp(35000);
    calcTemp(34000);
}
