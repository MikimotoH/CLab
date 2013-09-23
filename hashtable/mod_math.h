#pragma once
#include "pi_utils.h"
#include "hashtable.h"

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

static inline u16 mod(int x)
{
    int ret = int32mod(x, g_hashtable_cap);
    PI_ASSERT(ret < g_hashtable_cap && ret >=0);
    return (u16)ret;
}

static inline u16 mod_inc(u16 h)
{
   ++h;
   return h != g_hashtable_cap ? h : 0;
}

// @return (a - b) mod n
// if mod_number==257 ;then
//   mod_dif(0, 256) == 1
//   mod_dif(256, 0) == 256
//   mod_dif(256, 255) == 1
//   mod_dif(255, 256) == 256
//   mod_dif(1, 0) == 1
//   mod_dif(0, 1) == 256
//   mod_dif(-1, 255) == 1
//   mod_dif(255, -1) == 256
//   mod_dif(257, 256) == 1
//   mod_dif(256, 257) == 256
//   mod_dif(0, -1) == 1
//   mod_dif(-1, 0) == 256
static inline int32_t int32mod_dif(int32_t a, int32_t b)
{
    int32_t r = int32mod(a-b, g_hashtable_cap);
    PI_ASSERT(r>=0 && r<g_hashtable_cap);
    return r;
}

static inline u16 mod_dif(u16 a, u16 b)
{
    return (u16)int32mod_dif((int32_t)a, (int32_t)b);
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

