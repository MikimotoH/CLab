#pragma once

#ifdef _KERNEL 
#include <machine/stdarg.h>
#include <sys/_stdint.h>
#else
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#endif

/**
 * Class BufferedStringStream
 *  Usage: make sprintf()-strings easier to be appended
 */ 
typedef struct _BufferedStringStream{
    char* buf;
    uint16_t cap;
    uint16_t len;
} BufferedStringStream;

static inline void BSS_Init(BufferedStringStream* self, char* buf, uint16_t sz)
{
    self->buf = buf;
    self->cap = sz;
    self->len=0;
}
static inline int BSS_avail(const BufferedStringStream* self)
{
    if(self->cap < self->len)
        return 0;
    return self->cap - self->len;
}
static inline int BSS_printf(BufferedStringStream* self, const char* fmtStr, ...)
{
    if( BSS_avail(self) == 0)
        return 0;
    va_list args;
    va_start(args, fmtStr);

    int written = 
        vsnprintf(&self->buf[self->len], self->cap - self->len, fmtStr, args);
    self->len += (uint16_t)written;
    va_end(args);
    return written;
}
static inline int BSS_print(BufferedStringStream* self, const char* str)
{
    if( BSS_avail(self) == 0)
        return 0;
    int written = strlcat(&self->buf[self->len], str, self->cap - self->len);
    self->len += written;
    return written;
}

