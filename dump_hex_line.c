
#include <stdio.h>
#include "pi_string_utils.h"

#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <sys/param.h>

typedef struct{
    char str[256];
} str256byte_t;

str256byte_t
binary_to_hex_str256( const void* data, size_t len)
{
    str256byte_t ret={{0}};
    const uint8_t* bytes = (const uint8_t*)data;
    size_t actualBytes = MIN(len, (sizeof(ret.str)-1)/2 );
    for(size_t i=0; i<actualBytes; ++i)
    {
        snprintf(ret.str+i*2, sizeof(ret.str)-i*2, "%02X", bytes[i]);
    }
    return ret;
}

static void pi_dump_hex_line(const void* mem, size_t len)
{
    const uint8_t* pbeg = (const uint8_t*)mem;
    const uint8_t* pend = pbeg+len;
    const uint8_t* p; 
    enum{
        WIDTH=8,
    };
    for(p=pbeg; p<pend; p += WIDTH)
    {
        char buf[2+6+2+3*WIDTH+2+1*WIDTH+2] = {0};
        int i=0;        
        size_t w = pend-p >= WIDTH ? WIDTH : (pend-p);
        BufferedStringStream bss={0};
        BSS_Init(&bss, buf, sizeof(buf));
        BSS_printf(&bss, "0x%06X:  ", p-pbeg);
        for(i=0; i<w; ++i)
            BSS_printf(&bss, "%02X ", p[i]);
        for(; i< WIDTH; ++i)
            BSS_print(&bss, "   ");

#define PR(c) (c>=' '&& c<=0x7f) ? c : '.' 
        for(i=0; i<w; ++i)
            BSS_printf(&bss, "%c", PR(p[i]));
        for(; i< WIDTH; ++i)
            BSS_print(&bss, " ");
#undef  PR
        printf("%s\n", buf);
    }
}



int main(){

    uint8_t data[256]={0};//2,3,4,6,7,8};//  {0,'\n', '\r', 'H','e','l','l', 'o', 'W', 'O', 'R', 'd', 0xFF, 0xE1, 222};

    for(size_t i=0; i<sizeof(data); ++i)
        data[i] = (uint8_t)i;

    str256byte_t str = binary_to_hex_str256(data, sizeof(data));
    printf("strlen=%lu\n", strlen(str.str));
    printf("data[%d] = %s\n", sizeof(data), binary_to_hex_str256(data, sizeof(data)).str );
    //printf("data size=%lu\n", sizeof(data));
    //pi_dump_hex_line(data, sizeof(data));

    return 0;
}
