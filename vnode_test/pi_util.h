#pragma once
#include <sys/cdefs.h>
#include <sys/stdint.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/endian.h>
#include <sys/types.h>


#include <sys/lock.h>
#include <sys/kernel.h>
#include <sys/queue.h>
#include <sys/rman.h>
#include <sys/malloc.h>
#include <sys/mutex.h>
#include <sys/condvar.h>
#include <sys/rwlock.h>

#include <sys/module.h>
#include <sys/linker.h>
#include <sys/firmware.h>

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/proc.h>
#include <sys/bus.h>
#include <sys/unistd.h>
#include <sys/kthread.h>
#include <sys/sched.h>
#include <sys/smp.h>

//#include <dev/pci/pcireg.h>
//#include <dev/pci/pcivar.h>
//
//#include <machine/bus.h>
#include <machine/cpu.h>
#include <machine/atomic.h>

#ifndef IN
#define IN
#endif

#ifndef OUT
#define OUT
#endif

#ifndef OPTIONAL
#define OPTIONAL
#endif

#define likely(x)                       __builtin_expect(!!(x), 1)
#define unlikely(x)                     __builtin_expect(!!(x), 0)
#define typeof(x)                       __typeof(x)

#define LETVAR(newvar,rhs)              typeof(rhs) newvar = (rhs)

static inline uint8_t
GETBYTE(const void *h, uint32_t offset)
{
    return ((const uint8_t *)h)[offset];
}

static inline uint8_t GETHINIBBLE(uint8_t b)
{
    return (uint8_t)((b>>4) & 0xF);
}
static inline uint8_t GETLONIBBLE(uint8_t b)
{
    return (uint8_t)(b & 0xF);
}

#define NIBBLES2BYTE(hi_ni, low_ni) (uint8_t)(((hi_ni & 0xF)<<4) | (low_ni & 0xF))

typedef struct
{
    uint8_t b[3];
} __packed uint24_t;

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint24_t u24;
typedef uint32_t u32;
typedef uint64_t u64;
typedef unsigned __int128 u128;

typedef struct
{
    u8 b[1];
} __packed be8;

typedef struct
{
    u8 b[2];
} __packed be16;

typedef struct
{
    u8 b[3];
} __packed be24;

typedef struct
{
    u8 b[4];
} __packed be32;

typedef struct
{
    u8 b[8];
} __packed be64;


static inline u32 saturated_u64tou32( u64 q )
{
    return (q>0xFFFFFFFFuLL)? 0xFFFFFFFFuL: (u32)(q);
}
static inline u24 overflowed_u32tou24(u32 d)
{
    u24 r;
    __builtin_memcpy(&r, &d, 3);
    return r;
}
static inline u32 u24tou32(u24 d)
{
    u32 r=0;
    __builtin_memcpy(&r, &d, 3);
    return r;
}


#define SWAP(x,y) ( {__typeof__(x) z = x ; x = y; y = z;}  )
#define SWAPU24(u) ({ u24 v= *(u24*)&u; SWAP(v.b[0], v.b[1]); v; })

static inline u24 bswap24(u24 u)
{
    LETVAR(b, (u8 *)&u);
    SWAP(b[0], b[2]);
    return u;
}
//#define htobe24 bswap24
//#define be24toh bswap24

static inline u64 pi_bswap64(u64 u)
{
    return bswap64(u);
}
static inline u32 pi_bswap32(u32 u)
{
    return bswap32(u);
}
static inline u16 pi_bswap16(u16 u)
{
    return bswap16(u);
}
static inline  u8 pi_bswap8(  u8 x)
{
    return x;
}

#define xtypecast(type,var) (*((type*)&(var)))
static inline u64 pi_be64toh(be64 x)
{
    return bswap64(xtypecast(u64,x));
}
static inline u32 pi_be32toh(be32 x)
{
    return bswap32(xtypecast(u32,x));
}
static inline u24 pi_be24toh(be24 x)
{
    return bswap24(xtypecast(u24,x));
}
static inline u16 pi_be16toh(be16 x)
{
    return bswap16(xtypecast(u16,x));
}
static inline  u8 pi_be8toh(be8 x)
{
    return xtypecast(u8,x);
}

static inline be64 pi_htobe64(u64 x)
{
    LETVAR(y,bswap64(x));
    return xtypecast(be64,y);
}
static inline be32 pi_htobe32(u32 x)
{
    LETVAR(y,bswap32(x));
    return xtypecast(be32,y);
}
static inline be24 pi_htobe24(u24 x)
{
    LETVAR(y,bswap24(x));
    return xtypecast(be24,y);
}
static inline be16 pi_htobe16(u16 x)
{
    LETVAR(y,bswap16(x));
    return xtypecast(be16,y);
}
static inline  be8 pi_htobe8(u8 x)
{
    return xtypecast(be8,x);
}

/*
 * C11 type Generic
 * like C++ function overloading if compiler can known argument type at comple-time
 */
#define pi_betoh(n) \
    _Generic((n), \
             be64: pi_be64toh,\
             be32: pi_be32toh,\
             be24: pi_be24toh,\
             be16: pi_be16toh,\
             be8:  pi_be8toh )\
    (n)

#define pi_htobe(n) \
    _Generic((n), \
             u64: pi_htobe64,\
             u32: pi_htobe32,\
             u24: pi_htobe24,\
             u16: pi_htobe16,\
             u8:  pi_htobe8 )\
    (n)

#define likely(x)                       __builtin_expect(!!(x), 1)
#define unlikely(x)                     __builtin_expect(!!(x), 0)
#define typeof(x)                       __typeof(x)

/*
 * // copied from linux/include/linux/kernel.h
 * min()/max()/clamp() macros that also do
 * strict type-checking.. See the
 * "unnecessary" pointer comparison.
 * It can avoid comparing uint and int.
 * e.g.:
 * {
 *      unsigned int x = 1;
 *      int          y = -1;
 *      z = max(x,y) // z is expected to be 1, not -1.
 * }
 * C99 6.5.9.2 says that for the == comparison:
 *     Both operands are pointers to qualified or unqualified version of
 *     compatible types
 */
#define __min(x, y) ({				\
        typeof(x) _min1 = (x);			\
        typeof(y) _min2 = (y);			\
        (void) (&_min1 == &_min2);		\
        _min1 < _min2 ? _min1 : _min2; })

#define __max(x, y) ({				\
        typeof(x) _max1 = (x);			\
        typeof(y) _max2 = (y);			\
        (void) (&_max1 == &_max2);		\
        _max1 > _max2 ? _max1 : _max2; })

#define HASBIT(data,bit) ( ((data) & (bit)) != 0 )
#define NOBIT(data,bit) ( ((data) & (bit)) == 0 )

#define ARRAY_SIZE(x)   (sizeof(x) / sizeof((x)[0]))

// __noop() is already builtin in clang
//#define __noop(x) do {} while(0)

#define MEMCPYBYTEARRAY(dest, ...) \
    ({ \
        uint8_t arr[]={ __VA_ARGS__ }; \
        __builtin_memcpy(dest, arr, sizeof(arr) ); \
        sizeof(arr); \
    })

/*
 * @brief check whether arg_x is in the list {arg_a, ...}
 * @param arg_x:  the element to find
 * @param arg_a:  first element in the list
 * @param ...    the rest elements of list
 */
#define __is_in(arg_x, arg_a, ...) \
    ({                                              \
        LETVAR(x, arg_x);                              \
        typeof(arg_a) a_list[] = {arg_a,  __VA_ARGS__ };  \
        LETVAR(list_len, ARRAY_SIZE(a_list));       \
        LETVAR(i, list_len);                        \
        for( i=0; i< list_len; ++i){                \
            if( x == a_list[i] )                    \
                break;                              \
        }                                           \
        i != list_len;                              \
    })

#define __is_in2(arg_x, a, b) (arg_x==a || arg_x==b)

#define __is_in3(arg_x, a, b, c) ((arg_x==a) || (arg_x==b) || (arg_x==c))


#define __overloadable __attribute__((overloadable))
#define __selectany    __declspec(selectany)

#define BZERO(x) __builtin_memset(&x, 0, sizeof(x));


static inline bool __overloadable
string_equal(const char *str1, const char *str2)
{
    return __builtin_strcmp(str1, str2)==0;
}
static inline bool __overloadable
string_starts_with(const char *str, const char *prefix)
{
    u32 prefix_len = __builtin_strlen(prefix);
    return __builtin_strncmp(str, prefix, prefix_len);
}

/*
 *  x>=0 && x<H
 */
#define RANGES_BETWEEN(x, L, H) \
    (((x)>=(L)) && ((x)<(H)))

#define __cleanup(func)  __attribute__((cleanup(func)))

#define TYPECAST(type,var) ((type)(var))

