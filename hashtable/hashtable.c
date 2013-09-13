#include <sys/cdefs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <time.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdbool.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef unsigned __int128 u128;
#define LOGERR(fmtstr, ...)  do{ printf("<ERROR>[%s]" fmtstr "\n", __func__, ## __VA_ARGS__);} while(0)
#define LOGDBG(fmtstr, ...)  do{ printf("<DEBUG>[%s]" fmtstr "\n", __func__, ## __VA_ARGS__);} while(0)
#define ARRAY_SIZE(x)   (sizeof(x) / sizeof((x)[0]))
#define typeof(x)                       __typeof(x)
#define LETVAR(newvar,rhs)              typeof(rhs) newvar = (rhs) 

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

#define key_tostr(key) \
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

#define value_tostr(pvalue) \
    ({ char str[256];\
     sprintf(str, "{key=0x%016lx, gen=%u}", pvalue->key, pvalue->generation);\
     str;\
     })

typedef struct{
    itnexus_t  key;
    pr_reg_t*  value;
    union{
        u32 cluster_dense;
        u32 is_valid;
    };
    u32 cluster_name;
}entry_t;

#define key_equal(key1, key2)  (memcmp((key1).b, (key2).b, sizeof((key1).b)) == 0)

enum{
    hashtable_cap=257,
};
static entry_t g_hashtable[hashtable_cap];
static int hashtable_valid=0;
/**
 * http://en.wikipedia.org/wiki/Modular_arithmetic
 */
int mod(int x, int y){
    if(y<0)
        return (exit(-1), printf("denominator y=(%d) can't be negative",  y), -1);
    if(x>=0)
        return x%y;

    return y - (-x)%y;
}
static inline u32 wrapinc(u32 x){
    ++x;
    if(x == hashtable_cap)
        x=0;
    return x;
}
static inline u32 wrapdec(u32 x){
    --x;
    if(x == -1)
        x=hashtable_cap-1;
    return x;
}

u32 hash_multiplicative(const u8 key[], size_t len) {
    enum{
        INITIAL_VALUE = 5381 ,
        M = 33,
    };
   u32 hash = INITIAL_VALUE;
   for(size_t i = 0; i < len; ++i)
      hash = M * hash + key[i];
   return hash % hashtable_cap;
}
#define hash_func(key) hash_multiplicative(key.b, sizeof(key.b))



entry_t* table_alloc(itnexus_t key)
{
    if( hashtable_valid==hashtable_cap-2){
        LOGERR("hashtable is full, alloc failed");
        return NULL;
    }
    u32 h = hash_func(key);
    u32 hbeg = h;
    int visited=0;

    do{
        entry_t* e = &g_hashtable[h];
        if( !e->is_valid ){
            e->key = key;
            e->is_valid = 1;
            e->cluster_name = hbeg;
            ++hashtable_valid;
            return e;
        }
        if( key_equal(e->key,  key) ){
            printf("repeat alloc same key=%s\n", key_tostr(key));
            return e;
        }
        // cluster occurs. cluster grows
        if(e->cluster_name == hbeg)
            e->cluster_dense++;

        h = wrapinc(h);
        ++visited;
        if(visited == hashtable_valid){
            printf("visited=%d alloc failed\n", visited);
            break;
        }
    }while(h != hbeg );
    printf("h has wheel a round, alloc fail\n");
    return NULL;
}

entry_t* table_find(itnexus_t key){
    if( hashtable_valid==0 ){
        printf("hashtable is empty, find failed\n");
        return NULL;
    }
    u32 h = hash_func( key); 
    u32 hbeg = h;
    u32 visited=0;

    do{
        entry_t* e = &g_hashtable[h];
        if( e->is_valid && key_equal(e->key,key)){
            LOGDBG("key=%s visited =%u", key_tostr(key), visited);
            return e;
        }
        h = wrapinc(h);
        ++visited;
        if(visited == hashtable_valid){
            LOGERR("visited=%u find failed", visited);
            break;
        }
    }while(h != hbeg );

    LOGERR("key=%s h=%u has wheel a round, find fail", key_tostr(key), h);
    return NULL;
}

void table_foreach(int (*func)(entry_t*, void*), void* args){
    int i=0;
    for(; i<hashtable_cap; ++i){
        entry_t* e = &g_hashtable[i];
        if(e->is_valid){
            int cont = (func)(e, args);
            if(!cont)
                break;
        }
    }
}
double table_load_factor(){
    return ((double)hashtable_valid)/((double)hashtable_cap);
}

void table_stats(){
    for(int i=0; i< hashtable_cap; ++i){
        entry_t* e = &g_hashtable[i];
        if(!e->is_valid)
            continue;
        LOGDBG("[%d]:{cluster_dense=%u, cluster_name=%u, key=%s, value=%s}\n",
                i, e->cluster_dense, e->cluster_name, key_tostr(e->key), value_tostr(e->value) );
    }
    LOGDBG("load_factor=%f", table_load_factor());
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
const pr_reg_t* table_get(itnexus_t key){
    entry_t* e = table_find(key);
    if(!e){
        LOGERR("table_find %s failed", key_tostr(key));
        return NULL;
    }
    return e->value;
}

int visitor(entry_t* e, void* args)
{
    LETVAR(arg_key, (itnexus_t*)args);
    if(key_equal(e->key, *arg_key)){ 
        printf("key=%s,value='%s'\n", key_tostr(e->key), value_tostr(e->value)); 
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
#define u32random ((u64)((u32)random()))
            u64 rk = u32random << 32 | u32random;
            table_assign( make_itnexus( iports[i], tports[t] ), make_pr_reg( rk ) );
        }
    }

    //table_assign(make_itnexus(0x2001000e1e09f268, 0x2001000e1e09f290), make_pr_reg(0x000000001234abcd));
    //table_assign(make_itnexus(0x2001000e1e09f269, 0x2001000e1e09f291), make_pr_reg(0xA));
    //table_assign(make_itnexus(0x2001000e1e09f278, 0x2001000e1e09f292), make_pr_reg(0xb));
    //table_assign(make_itnexus(0x2001000e1e09f279, 0x2001000e1e09f293), make_pr_reg(0xc));
    //table_assign(make_itnexus(0x2100000e1e116080, 0x2001000e1e09f294), make_pr_reg(0xd));
    //table_assign(make_itnexus(0x2100000e1e116081, 0x2001000e1e09f295), make_pr_reg(0xe));
    //table_assign(make_itnexus(0x2001000e1e09f282, 0x2001000e1e09f296), make_pr_reg(0x8));
    //table_assign(make_itnexus(0x2001000e1e09f283, 0x2001000e1e09f297), make_pr_reg(0x9));
    table_stats();
    (void)table_find(make_itnexus(0x2001000e1e09f20aULL, 0x0100000e1e116080));
    (void)table_find(make_itnexus(0x0001000e1e09f20aULL, 0x2100000e1e116080));
    (void)table_find(make_itnexus(0x2001000e1e09f20aULL, 0x2100000e1e116080));
    (void)table_find(make_itnexus(0x2001000e1e09f203ULL, 0x2001000e1e09f268));

    itnexus_t arg_key = make_itnexus(0x2001000e1e09f268, 0x2001000e1e09f200);
    table_foreach( visitor, &arg_key);

    return 0;
}

