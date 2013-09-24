#include "bitmap.h"


void unittest_bitmap()
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

