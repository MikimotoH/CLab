#ifndef __vector_h__
#define __vector_h__
#include <sys/cdefs.h>
#include <stdbool.h>
#include "pi_utils.h"

#ifndef VEC_ELEM_TYPE
#error "VEC_ELEM_TYPE must be defined before include " __FILE__
#endif

#define DEFINE_VECTOR(T) \
typedef struct{\
    u16      cap; \
    u16      size; \
    T*  data; \
} __packed __CONCAT(vector_, T) 

DEFINE_VECTOR(VEC_ELEM_TYPE);
#define VECTOR_T __CONCAT(vector_, VEC_ELEM_TYPE)

static inline bool  __overloadable
vector_init(VECTOR_T* self, u16 cap)
{
    PI_ASSERT(cap>0);
    BZERO(*self);
    self->data = (VEC_ELEM_TYPE*)malloc(sizeof(VEC_ELEM_TYPE)*cap);
    if(!self->data){
        LOGERR("malloc(%lu bytes) failed", sizeof(VEC_ELEM_TYPE)*cap );
        return false;
    }

    self->cap = cap;
    return true;
}

static inline bool __overloadable
vector_growup(VECTOR_T* self, u16 newcap)
{
    PI_ASSERT(newcap > self->cap);
    self->data = (VEC_ELEM_TYPE*)reallocf(self->data, 
            sizeof(VEC_ELEM_TYPE)*newcap);
    if(!self->data){
        LOGERR("reallocf(%lu bytes) failed.",
                sizeof(VEC_ELEM_TYPE)*newcap);
        return false;
    }
    self->cap = newcap;
    return true;
}

#ifndef __vector_cleanup__
#define __vector_cleanup__
static inline void 
vector_cleanup(void* aself)
{
    DEFINE_VECTOR(void);
    vector_void* self = (vector_void*)(aself);
    if(self->data){
        free(self->data);
        self->data = NULL;
    }
    BZERO(*self);
}
#endif // __vector_cleanup__

#define __autofree __attribute((cleanup(vector_cleanup)))
    
static inline u16 __overloadable
vector_length(const VECTOR_T* self)
{
    return self->size;
}

static inline bool __overloadable
vector_is_empty(const VECTOR_T* self)
{
    return vector_length(self)==0;
}

static inline VEC_ELEM_TYPE* __overloadable
vector_get_at_index(const VECTOR_T* self, u16 index)
{
    PI_VERIFY(index < self->size );
    return &self->data[index];
}


static inline bool __overloadable
vector_append(VECTOR_T* self, VEC_ELEM_TYPE ent)
{
    if(self->cap == self->size ){
        u16 bigcap = self->cap*2;
        if(bigcap==0)
            bigcap=4;
        if( ! vector_growup(self, bigcap) ){
            LOGERR("vector_growup(%u) failed.", bigcap);
            return false;
        }
    }
    self->data[self->size] = ent;
    ++self->size;
    return true;
}

static inline bool __overloadable
vector_delete_at_index(VECTOR_T* self, u16 idx)
{
    PI_ASSERT(idx< self->size);
    for(u16 i=idx+1; i<self->size; ++i){
        SWAP( self->data[i-1], self->data[i] );
    }
    --self->size;
    return true;
}


static inline VEC_ELEM_TYPE  __overloadable
vector_get_first(VECTOR_T* self)
{
    PI_ASSERT(self->size>0);
    return self->data[0];
}
static inline bool __overloadable
vector_remove_head(VECTOR_T* self)
{
    PI_ASSERT(self->size > 0 );
    return vector_delete_at_index(self, 0);
}

#define vector_foreach(iter, vector) \
    for( LETVAR(iter, &(vector).data[0]); \
            iter != &(vector).data[(vector).size]; \
            ++iter)

#endif // __vector_h__

