#pragma once
#include "vector.h"
#include "random.h"
#include "itnexus.h"

static inline void
unittest_vector()
{
    vector_t v;
    PI_VERIFY(vector_init(&v, 7));

    PI_VERIFY( vector_append(&v, 
                make_pr_reg( 
                    make_fcp_itnexus(u64rand(), u64rand()), 
                    u64rand())));
}
