#include "pi_utils.h"
#include <stdlib.h>
#include <machine/_types.h>
#include <sys/types.h>
#include <machine/cpufunc.h>

typedef struct{
    u32 b[10];
}bitmap_t;

static inline void 
bitmap_init(bitmap_t* self)
{
    bzero(&self->b, sizeof(self->b));
}

static inline bool 
bitmap_check(bitmap_t* self, u16 index)
{
    div_t qr = div(index, 32);
    PI_ASSERT(qr.quot < ARRAY_SIZE(self->b));
    return (self->b[qr.quot] & (1<<qr.rem)) != 0;
}

static inline void 
bitmap_set(bitmap_t* self, u16 index)
{
    div_t qr = div(index, 32);
    PI_ASSERT(qr.quot < ARRAY_SIZE(self->b));
    (self->b[qr.quot] |= (1<<qr.rem));
}

static inline void 
bitmap_unset(bitmap_t* self, u16 index)
{
    div_t qr = div(index, 32);
    PI_ASSERT(qr.quot < ARRAY_SIZE(self->b));
    (self->b[qr.quot] &= ~(1<<qr.rem));

}

/*
 * @return first index of unset bit, searching from index=0
 *   to index = num_bits-1
 */
static inline u32 
bitmap_find_first_zero(bitmap_t* self)
{
    u32 i;
    for(i=0; i< ARRAY_SIZE(self->b); ++i) {
        if( self->b[i] != 0xFFFFFFFF ) {
            u32 b = ~(self->b[i]);
            u32 j = bsfl(b);
            return (u32)(i*32 + j);
        }
    }
    return (u32)(-1);
}

/*
 * @return first index of set bit, searching from index=0
 *   to index = num_bits-1
 */
static inline u32 
bitmap_find_first_set(bitmap_t* self)
{
    u32 i;
    for(i=0; i< ARRAY_SIZE(self->b); ++i) {
        if( self->b[i] != 0x00000000 ) {
            u32 b = (self->b[i]);
            u32 j = bsfl(b);
            return (u32)(i*32 + j);
        }
    }
    return (u32)(-1);
}

static inline void 
unittest_bitmap()
{
    bitmap_t bm;
    bitmap_init(&bm);
    bm.b[0] = 0xFFFFFFFF;
    bm.b[1] = 0xFFFFFFFF;
    bm.b[2] = 0xFFFFFFFF;
    PI_ASSERT(bitmap_find_first_set(&bm) == 0);
    PI_ASSERT(bitmap_find_first_zero(&bm) == 96);
    PI_ASSERT(bitmap_check(&bm, 83) == true);

    bitmap_unset(&bm, 83);
    PI_ASSERT(bitmap_check(&bm, 83) == false);
    PI_ASSERT(bitmap_find_first_zero(&bm) == 83);
    PI_ASSERT(bitmap_find_first_set(&bm) == 0);

    bm.b[0] = 0x00000000;
    bm.b[1] = 0x00000000;
    bm.b[2] = 0x00000000;
    bitmap_set(&bm, 83);
    PI_ASSERT(bitmap_check(&bm, 83) == true);
    PI_ASSERT(bitmap_find_first_set(&bm) == 83);
    PI_ASSERT(bitmap_find_first_zero(&bm) == 0);
}

