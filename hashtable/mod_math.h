#pragma once
#include "pi_utils.h"

static inline int32_t int32mod(int32_t x, int32_t y)
{
    PI_ASSERT(y>=2);
    if(x>=0)
        return x%y;

    int32_t r =  y - (-x)%y;
    if(r==y)
        r = 0;
    PI_ASSERT(r>=0 && r<y);
    return r;
}

#ifdef UNIT_TEST
static inline void unittest_int32mod_dif()
{
   PI_ASSERT(int32mod_dif(0, 256) == 1);
   PI_ASSERT(int32mod_dif(256, 0) == 256);
   PI_ASSERT(int32mod_dif(256, 255) == 1);
   PI_ASSERT(int32mod_dif(255, 256) == 256);
   PI_ASSERT(int32mod_dif(1, 0) == 1);
   PI_ASSERT(int32mod_dif(0, 1) == 256);
   PI_ASSERT(int32mod_dif(-1, 255) == 1);
   PI_ASSERT(int32mod_dif(255, -1) == 256);
   PI_ASSERT(int32mod_dif(257, 256) == 1);
   PI_ASSERT(int32mod_dif(256, 257) == 256);
   PI_ASSERT(int32mod_dif(0, -1) == 1);
   PI_ASSERT(int32mod_dif(-1, 0) == 256);
}
#endif//UNIT_TEST

