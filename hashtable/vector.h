#pragma once

typedef struct{
    u16      cap;
    u16      size;
    pr_reg_t*  data; // array of size = cap;
} vector_t;

static inline bool 
vector_init(vector_t* self, u16 cap)
{
    BZERO(*self);
    self->data = (pr_reg_t*)malloc(sizeof(pr_reg_t)*cap);
    if(!self->data) 
        return false;

    self->cap = cap;
    return true;
}
static inline void
vector_cleanup(vector_t* self)
{
    if(self->data){
        free(self->data);
        self->data = NULL;
    }
    BZERO(*self);
}
    
static inline u16
vector_length(const vector_t* self)
{
    return self->size;
}

static inline bool
vector_is_empty(const vector_t* self)
{
    return vector_length(self)==0;
}

static inline pr_reg_t*
vector_get_at_index(const vector_t* self, u16 index)
{
    PI_VERIFY(index < self->size );
    return &self->data[index];
}

static inline pr_reg_t*
vector_find(const vector_t* self, itnexus_t nexus)
{
    for(u16 i=0; i<self->size; ++i){
        if( itnexus_equal(self->data[i].nexus, nexus) )
            return &self->data[i];
    }
    return NULL;
}

static inline bool
vector_append(vector_t* self, pr_reg_t ent)
{
    if(self->cap == self->size ){
        LOGERR("vector is full, capacity=%u", self->cap);
        return false;
    }
    self->data[self->size] = ent;
    ++self->size;
    return true;
}

static inline bool 
vector_delete_at_index(vector_t* self, u16 idx)
{
    PI_ASSERT(idx< self->size);
    for(u16 i=idx+1; i<self->size; ++i){
        SWAP( self->data[i-1], self->data[i] );
    }
    --self->size;
    return true;
}
static inline bool 
vector_delete(vector_t* self, itnexus_t nexus)
{
    pr_reg_t* e = vector_find(self, nexus);
    if(e == NULL){
        LOGERR("Can NOT find nexus=%s", itnexus_tostr(nexus));
        return false;
    }
    u16 idx = (u16)(e - self->data);
    return vector_delete_at_index(self, idx);
}


static inline pr_reg_t
vector_get_first(vector_t* self)
{
    PI_ASSERT(self->size>0);
    return self->data[0];
}
static inline bool
vector_remove_head(vector_t* self)
{
    PI_ASSERT(self->size > 0 );
    return vector_delete_at_index(self, 0);
}

