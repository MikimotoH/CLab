#undef  VEC_ELEM_TYPE 
#define VEC_ELEM_TYPE u32
#undef __vector_h__
#include "vector.h"
#undef VEC_ELEM_TYPE

#define VEC_ELEM_TYPE double
#undef __vector_h__
#include "vector.h"
#undef VEC_ELEM_TYPE

#include "random.h"


static inline void
unittest_vector()
{
    __autofree vector_u32 v={0};
    __autofree vector_double u={0};
    PI_VERIFY(vector_init(&v, 4));
    PI_VERIFY(vector_init(&u, 4));
    PI_VERIFY(v.data != NULL);
    PI_VERIFY(v.cap == 4);
    PI_VERIFY(v.size == 0);

    for(u16 i=0; i<4; ++i){
        PI_VERIFY( vector_append(&v, i+1 ));
        PI_VERIFY( vector_append(&u, i+1 ));
        PI_VERIFY(v.size == i+1 );
    }
    PI_VERIFY(vector_append(&v, 5));
    PI_VERIFY(v.cap == 8);
    PI_VERIFY(v.size == 5);


    for(u16 i=0; i<5; ++i){
        PI_VERIFY(v.data[0]== i+1 );
        vector_delete_at_index(&v, 0);
        PI_VERIFY( v.size== 5-i-1 );
    }

}

int main()
{
    unittest_vector();
    return 0;
}

