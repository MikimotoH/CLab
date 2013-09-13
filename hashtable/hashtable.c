#include <sys/cdefs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <time.h>
#include <assert.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdbool.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef unsigned __int128 u128;
#define LOGERR(fmtstr, ...)  \
    do{ printf("<ERROR>[%s]" fmtstr "\n", __func__, ## __VA_ARGS__);\
    } while(0)

#define LOGINF(fmtstr, ...)  \
    do{ printf("<INFO>[%s]" fmtstr "\n", __func__, ## __VA_ARGS__);\
    } while(0)
#define LOGDBG(fmtstr, ...)  \
    do{ printf("<DEBUG>[%s][%u]" fmtstr "\n", __func__, __LINE__, ## __VA_ARGS__);\
    } while(0)
#define LOGTRC(fmtstr, ...)  \
    do{ printf("<TRACE>[%s][%u]" fmtstr "\n", __func__, __LINE__, ## __VA_ARGS__);\
    } while(0)

#define ARRAY_SIZE(x)   (sizeof(x) / sizeof((x)[0]))
#define typeof(x)                       __typeof(x)
#define LETVAR(newvar,rhs)              typeof(rhs) newvar = (rhs) 

#define __max(x, y) ({				\
	typeof(x) _max1 = (x);			\
	typeof(y) _max2 = (y);			\
	(void) (&_max1 == &_max2);		\
	_max1 > _max2 ? _max1 : _max2; })
typedef union{
    u8 b[8];
    uint64_t qword;
} wwpn_t;
_Static_assert(sizeof(wwpn_t)==8,"");

typedef union{
    struct{
        wwpn_t i;
        wwpn_t t;
    };
    u8 b[16];
    u128 oword;
} itnexus_t;
_Static_assert(sizeof(itnexus_t)==16, "");

#define itnexus_tostr(key) \
    ({\
        char str[ 16*2+6+2+2 ] ;\
        sprintf(str, "\"%016lx <=> %016lx\"", key.i.qword, key.t.qword);\
        str;\
    })

#define make_itnexus(iport, tport) (itnexus_t){.i.qword=iport, .t.qword=tport}

typedef struct{
    u8 rsv_type;
    u8 scope;
    u64 key;
    u32 generation;
    u32 vol_id;
    u8 aptpl;
    u8  mapped_lun_id;
} pr_reg_t;

static u32 g_pr_generation = 0;

#define make_pr_reg(arg_key) (pr_reg_t){ .rsv_type=0, .scope=0, .key=arg_key, \
    .generation= g_pr_generation++, .vol_id=0, .aptpl=0, .mapped_lun_id = 0 }

#define pr_reg_tostr(pvalue) \
    ({ char str[256];\
     sprintf(str, "{key=0x%016lx, gen=%u}", pvalue->key, pvalue->generation);\
     str;\
     })

typedef struct{
    itnexus_t  key;
    pr_reg_t*  value;
    u32 is_valid;
    union{
        u32 cluster_name;
        u32 hashvalue;
    };
    u32 nextsamehash;
}entry_t;

#define key_equal(key1, key2)  (memcmp((key1).b, (key2).b, sizeof((key1).b)) == 0)

enum{
    g_hashtable_cap= 257,
};
static entry_t g_hashtable[g_hashtable_cap];
static int g_hashtable_valid=0;
/**
 * http://en.wikipedia.org/wiki/Modular_arithmetic
 */
int mod(int x, int y){
    assert(y>=2);
    if(x>=0)
        return x%y;

    return y - (-x)%y;
}
u32 modu32(u32 x, u32 y){
    assert(y>=2);
    return x%y;
}

static inline u32 wrapinc(u32 x){
    ++x;
    if(x == g_hashtable_cap)
        x=0;
    return x;
}
static inline u32 wrapdec(u32 x){
    --x;
    if(x == -1)
        x=g_hashtable_cap-1;
    return x;
}

u32 hash_multiplicative(const u8* key, size_t len, u32 INITIAL_VALUE, u32 M)
{
   u32 hash = INITIAL_VALUE;
   for(size_t i = 0; i < len; ++i)
      hash = M * hash + key[i];
   return hash ;
}
#define hash_func1(key) \
    (hash_multiplicative((const u8*)(&(key)), sizeof(key), 5381, 33) % g_hashtable_cap)

u32 hash_func_naiive(itnexus_t key)
{
     return key.oword % g_hashtable_cap;
}

u32 hash_mul2(const itnexus_t key, u32 INITIAL_VALUE, u32 M)
{
    u32* dw = (u32*)&key;
    size_t len = sizeof(key)/sizeof(u32);
    u32 hash = INITIAL_VALUE;
    for(size_t i = 0; i < len; ++i)
        hash = M * hash + dw[i];
    return hash ;
}
#define hash_func2(key) \
    (hash_mul2(key, 5381, 33) % g_hashtable_cap)

#define hash_func  hash_func1



entry_t* table_alloc(itnexus_t key)
{
    if( g_hashtable_valid==g_hashtable_cap-2){
        LOGERR("hashtable is full, alloc failed");
        return NULL;
    }

    u32 h = hash_func(key);
    u32 horig = h;
    entry_t* e = NULL;
    entry_t* prev_e = e;
    u32 visited = 0;
    u32 nextsamehash = 0;

    do
    {
        e = &g_hashtable[h];
        if( !e->is_valid )
            goto found_empty_slot;
        else if( key_equal(e->key,  key) ){
            LOGDBG("repeat alloc same key=%s", itnexus_tostr(key));
            return e;
        }
        
        // cluster occurs. cluster grows
        if(e->cluster_name == horig){
            prev_e = e;
            nextsamehash = 0;
        }
        
        // step next
        h = wrapinc(h), ++visited, ++nextsamehash;
    }
    while( visited < g_hashtable_valid );



found_empty_slot:
    if( !e->is_valid ){
        e->key = key;
        e->is_valid = 1;
        e->cluster_name = horig;
        ++g_hashtable_valid;
        if(prev_e)
            prev_e->nextsamehash = nextsamehash;
        return e;
    }
    LOGERR("visited=%d alloc failed", visited);
    return NULL;
}

entry_t* table_find(itnexus_t key)
{
    if( g_hashtable_valid==0 ){
        LOGINF("hashtable is empty, find failed");
        return NULL;
    }
    u32 h = hash_func(key);
    entry_t* e = NULL;
    u32 visited=0;

    LOGTRC("h=%u",h);

    do{
        e = &g_hashtable[h];
        if( e->is_valid &&  key_equal(e->key, key)){
            LOGTRC("e->is_valid &&  key_equal(e->key, key) found_key");
            goto found_key;
        }

        if( !e->is_valid ){
            LOGTRC("if( !e->is_valid) not_found_key ");
            goto not_found_key;
        }

        // step next
        if(e->nextsamehash == 0 ){
            LOGTRC("e->nextsamehash == 0  not_found_key");
            goto not_found_key;
        }
        h = ( h + e->nextsamehash) % g_hashtable_cap;
        LOGTRC("h=%u",h);
        ++visited;
        LOGTRC("visited=%u", visited);
    }while(visited < g_hashtable_valid);

found_key:
    if( e->is_valid && key_equal(e->key, key)){
        LOGDBG("key=%s visited=%u", itnexus_tostr(key), visited);
        return e;
    }
not_found_key:
    LOGINF("key=%s not found, visited=%u", itnexus_tostr(key), visited);
    if(visited == g_hashtable_valid)
        LOGERR("visited=%u, (g_hashtable_valid=%u) find failed", 
                visited, g_hashtable_cap);

    return NULL;
}

void table_foreach(int (*func)(entry_t*, void*), void* args)
{
    int i=0;
    for(; i<g_hashtable_cap; ++i){
        entry_t* e = &g_hashtable[i];
        if(e->is_valid){
            int cont = (func)(e, args);
            if(!cont)
                break;
        }
    }
}

double table_load_factor()
{
    return ((double)g_hashtable_valid)/((double)g_hashtable_cap);
}

void table_stats(){
    LOGDBG("load_factor=%f", table_load_factor());

    u32 maxnextsamehash=0;
    for(u32 i=0; i< g_hashtable_cap; ++i){
        entry_t* e = &g_hashtable[i];
        if(!e->is_valid)
            continue;
        maxnextsamehash = __max( maxnextsamehash, e->nextsamehash);
        LOGINF("[%u]:{cluster_name=%u, nextsamehash=%u\n"
                "   key=%s, value=%s }",
                i, e->cluster_name, e->nextsamehash,
                itnexus_tostr(e->key), pr_reg_tostr(e->value) );
    }
    LOGINF("maxnextsamehash = %u", maxnextsamehash);
}

bool table_assign(itnexus_t key, pr_reg_t value){
    entry_t* e = table_alloc(key);
    if(!e){
        LOGERR("table_alloc failed");
        return false;
    }
    e->value = malloc(sizeof(typeof(value)));
    memcpy(e->value, &value, sizeof(typeof(value)));

    return true;
}

pr_reg_t* table_get(itnexus_t key){
    entry_t* e = table_find(key);
    if(!e){
        LOGERR("table_find %s failed", itnexus_tostr(key));
        return NULL;
    }
    return e->value;
}

int visitor(entry_t* e, void* args)
{
    LETVAR(arg_key, (itnexus_t*)args);
    if(key_equal(e->key, *arg_key)){ 
        LOGINF("key=%s,value='%s'", itnexus_tostr(e->key), 
                pr_reg_tostr(e->value)); 
        return 0;
    }else{
        return 1;
    } 
} 

int main(int argc, char** argv){
    u64 tports[8]= {
        0x2001000e1e09f268ULL, 
        0x2001000e1e09f269ULL, 
        0x2001000e1e09f278ULL, 
        0x2001000e1e09f279ULL, 
        0x2100000e1e116080ULL, 
        0x2100000e1e116081ULL, 
        0x2001000e1e09f282ULL, 
        0x2001000e1e09f283ULL, 
    };
    u64 iports[16];
    for(size_t i=0; i< ARRAY_SIZE(iports); ++i){
        iports[i] = 0x2001000e1e09f200ULL + i;
    }
    srandom(time(NULL));
    for(size_t t=0; t< ARRAY_SIZE(tports); ++t){
        for(size_t i=0; i< ARRAY_SIZE(iports); ++i){
#define u32random ((u64)(u32)random())
            u64 rk = u32random << 32 | u32random;
            table_assign( make_itnexus( iports[i], tports[t] ), make_pr_reg( rk ) );
        }
    }

    table_stats();
    //(void)table_find(make_itnexus(0x2001000e1e09f20aULL, 0x0100000e1e116080));
    //(void)table_find(make_itnexus(0x0001000e1e09f20aULL, 0x2100000e1e116080));
    //(void)table_find(make_itnexus(0x2001000e1e09f20aULL, 0x2100000e1e116080));
    //(void)table_find(make_itnexus(0x2001000e1e09f203ULL, 0x2001000e1e09f268));
    (void)table_find(make_itnexus(0x2001000e1e09f205, 0x2001000e1e09f282));

    itnexus_t arg_key = make_itnexus(0x2001000e1e09f268, 0x2001000e1e09f200);
    table_foreach( visitor, &arg_key);

    return 0;
}

