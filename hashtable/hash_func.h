#pragma once
#include "pi_utils.h"

static inline u32 hash_byte_array(const u8* key, size_t len, 
        u32 INITIAL_VALUE /*=5381*/ )
{
   u32 hash = INITIAL_VALUE;
   for(size_t i = 0; i < len; ++i)
       hash = ((hash << 5) + hash) + key[i]; /* hash * 33 + c */
   return hash ;
}

static inline u32
hash_string(const char *str, u32 INITIAL_VALUE /*=5381*/ )
{
    u32 hash = INITIAL_VALUE;
    char c;
    while ((c = *str++) != 0)
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}


