
typedef struct {
    u32 cap;
    u32 num;
    T*  data;
} vector_t;

static inline __always_inline bool
vector_resize(vector_t* self, u32 newcap)
{
    LOGTRC("enter");
    self->data = KREALLOC(self->data, newcap * sizeof(T));
    if(!self->data){
        LOGCRI("KREALLOC(%lu bytes) failed", newcap * sizeof(T));
        self->cap = self->num = 0;
        return false;
    }
    self->cap = newcap;
    LOGTRC("return");
    return true;
}

static inline __always_inline void
vector_clear(vector_t* self)
{
    LOGTRC("enter");
    if(self->data)
        KFREE(self->data);
    self->data = NULL;
    self->cap = self->num = 0;
    LOGTRC("return");
}

static inline __always_inline bool __overloadable
vector_append(vector_t* self, T data)
{
    if(!self->data){
        if(!vector_resize(self, 2)){
            LOGCRI("vector_resize(%d) failed", 2);
            return false;
        }
    }
    if(self->num == self->cap){
        if(! vector_resize(self, self->cap * 2) )
            return false;
    }
    self->data[self->num++] = data;
    return true;
}

