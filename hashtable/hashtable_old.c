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
    u16 next_node;
    union{
        u16 cluster_name;
        u16 hashvalue;
    };
}entry_t;

#define key_equal(key1, key2)  (memcmp((key1).b, (key2).b, sizeof((key1).b)) == 0)

enum{
    g_hashtable_cap= 257,
};

static entry_t g_hashtable[g_hashtable_cap];
static int g_hashtable_valid=0;

static inline int mod_int(int x, int y){
    assert(y>=2);
    if(x>=0)
        return x%y;

    return y - (-x)%y;
}
static inline u16 mod(int x)
{
    int ret = mod_int(x, g_hashtable_cap);
    assert(ret < g_hashtable_cap && ret >=0);
    return (u16)ret;
}

u32 hash_multiplicative(const u8* key, size_t len, u32 INITIAL_VALUE, u32 M)
{
   u32 hash = INITIAL_VALUE;
   for(size_t i = 0; i < len; ++i)
      hash = M * hash + key[i];
   return hash ;
}

#define hash_func(key) \
    (u16)(hash_multiplicative((const u8*)(&(key)), sizeof(key), 5381, 33) \
            % g_hashtable_cap)



entry_t* table_alloc(itnexus_t key)
{
    if( g_hashtable_valid==g_hashtable_cap-2){
        LOGERR("hashtable is full, alloc failed");
        return NULL;
    }

    u16 h = hash_func(key);
    u16 orig_h = h;
    entry_t* e = NULL;
    u16 visited = 0;
    u16 prev_node = 0;

    do
    {
        e = &g_hashtable[h];
        if( !e->value )
            goto found_empty_slot;
        else if( key_equal(e->key,  key) ){
            LOGDBG("repeat alloc same key=%s", itnexus_tostr(key));
            return e;
        }
        // collision occurs 
        if(e->cluster_name == orig_h){
            LOGDBG("e->cluster_name(%hu) = orig_h, same cluster", e->cluster_name);
            // is same cluster
            if(e->next_node){
                // not tail of linked-list, hop
                prev_node = e->next_node;
                h = mod(h + prev_node);
            }
            else{
                // is tail of linked-list tail, step next
                assert(prev_node == 0);
                prev_node = 1;
                h = mod(h + 1);
            }
        }
        else
        {
            // invaded by different cluster
            LOGDBG("e->cluster_name=%hu, orig_h=%hu, invaded by different cluster", e->cluster_name, orig_h);
            // it is caused by neighbor cluster sprawl
            // step next
            prev_node +=1;
            h = mod(h + 1);
        }
        ++visited; 
    }
    while( visited < g_hashtable_valid );

found_empty_slot:
    if( !e->value ){
        e->key = key;
        e->cluster_name = orig_h;
        ++g_hashtable_valid;
        if(prev_node){
            entry_t* prev_e = &g_hashtable[ mod( h + prev_node ) ];
            prev_e->next_node = prev_node;
            e->next_node = 0;
        }
        return e;
    }
    LOGERR("visited=%d alloc failed", visited);
    return NULL;
}

entry_t* table_find_1(itnexus_t key, u16* out_prev_node, u16* out_index)
{
    if( g_hashtable_valid==0 ){
        LOGINF("hashtable is empty, find failed");
        return NULL;
    }
    u16 h = hash_func(key);
    entry_t* e = NULL;
    u16 visited=0;

    LOGTRC("h=%u",h);
    u16 prev_node = 0;

    do{
        e = &g_hashtable[h];
        if( e->value &&  key_equal(e->key, key)){
            LOGTRC("e->value != NULL &&  key_equal(e->key, key) found_key");
            goto found_key;
        }

        if( !e->value ){
            LOGTRC("if( !e->value) not_found_key ");
            goto not_found_key;
        }

        // step next
        if(e->next_node == 0 ){
            LOGTRC("e->next_node == 0  not_found_key");
            goto not_found_key;
        }
        h = mod( h + e->next_node);
        prev_node = e->next_node;
        LOGTRC("h=%u",h);
        ++visited;
        LOGTRC("visited=%u", visited);
    }while(visited < g_hashtable_valid);

found_key:
    if( e->value && key_equal(e->key, key)){
        LOGDBG("key=%s, index=%hu, visited=%u", itnexus_tostr(key), h, visited);
        *out_prev_node = prev_node;
        *out_index = h;
        return e;
    }
not_found_key:
    LOGINF("key=%s not found, visited=%u", itnexus_tostr(key), visited);
    if(visited == g_hashtable_valid)
        LOGERR("visited=%u, (g_hashtable_valid=%u) find failed", 
                visited, g_hashtable_cap);

    return NULL;
}

entry_t* table_find(itnexus_t key)
{
    u16 prev_node = 0;
    entry_t* e = table_find_1(key, &prev_node);
    return e;
}


bool table_remove(itnexus_t key)
{
    u16 prev_node = 0;
    entry_t* e = table_find_1(key, &prev_node);
    if(!e){
        LOGERR("table_find(key=%s) failed", itnexus_tostr(key));
        return false;
    }
    if(!e->value){
        LOGERR("e->value is NULL for key=%s", itnexus_tostr(key));
        return false;
    }
    u16 h = 
    entry_t* prev_e = &g_hashtable[ mod( h - prev_node) ];
    prev_e->next_node += e->next_node;

    return true;

}

void table_foreach(int (*func)(entry_t*, void*), void* args)
{
    u16 i=0;
    for(; i<g_hashtable_cap; ++i){
        entry_t* e = &g_hashtable[i];
        if(e->value){
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

void table_stats()
{
    LOGDBG("load_factor=%f", table_load_factor());

    u16 maxnextnode=0;
    for(u16 i=0; i< g_hashtable_cap; ++i){
        entry_t* e = &g_hashtable[i];
        if(!e->value)
            continue;
        maxnextnode = __max( maxnextnode, e->next_node);
        LOGINF("[%u]:{cluster_name=%u, next_node=%u\n"
                "   key=%s, value=%s }",
                i, e->cluster_name, e->next_node,
                itnexus_tostr(e->key), pr_reg_tostr(e->value) );
    }
    LOGINF("maxnextnode = %u", maxnextnode);
}

#define pod_dup(pod) \
    ({ \
        LETVAR(dup, (typeof(pod)*)malloc( sizeof(typeof(pod))));\
        dup ? (memcpy(dup, &pod, sizeof(typeof(pod))), dup)\
            : NULL;\
     })
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wincompatible-pointer-types-discards-qualifiers"
#define pod_ptr_dup(pod_ptr) \
    ({ \
        LETVAR(dup_ptr, (typeof(pod_ptr))malloc( sizeof(typeof( *pod_ptr))));\
        dup_ptr ? (memcpy(dup_ptr, pod_ptr, sizeof(typeof(*pod_ptr))), dup_ptr)\
            : NULL;\
     })
#pragma GCC diagnostic pop

bool table_assign(itnexus_t key, pr_reg_t value){
    entry_t* e = table_alloc(key);
    if(!e){
        LOGERR("table_alloc failed");
        return false;
    }
    e->value = pod_dup(value);

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
    (void)table_find(make_itnexus(0x2001000e1e09f20aULL, 0x0100000e1e116080));
    (void)table_find(make_itnexus(0x0001000e1e09f20aULL, 0x2100000e1e116080));
    (void)table_find(make_itnexus(0x2001000e1e09f20aULL, 0x2100000e1e116080));
    (void)table_find(make_itnexus(0x2001000e1e09f203ULL, 0x2001000e1e09f268));
    (void)table_find(make_itnexus(0x2001000e1e09f205, 0x2001000e1e09f282));

    itnexus_t arg_key = make_itnexus(0x2001000e1e09f268, 0x2001000e1e09f200);
    table_foreach( visitor, &arg_key);

    return 0;
}

