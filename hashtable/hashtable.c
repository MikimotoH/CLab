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
#include <sys/queue.h>

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
typedef struct{
    char str[ 16*2+6+2+2 ] ;
}itnexus_str_t;

static inline itnexus_str_t itnexus_tostr(itnexus_t key){
    itnexus_str_t ret;
    snprintf(ret.str, ARRAY_SIZE(ret.str), "\"%016lx <=> %016lx\"", key.i.qword, key.t.qword);
    return ret;
}

#define make_itnexus(iport, tport) (itnexus_t){.i.qword=iport, .t.qword=tport}

typedef struct pr_reg {
    itnexus_t nexus;
    u8 rsv_type;
    u8 scope;
    u64 key;
    u32 generation;
    u32 vol_id;
    u8 aptpl;
    u8  mapped_lun_id;
    // test for SLIST_HEAD
    SLIST_ENTRY(pr_reg) entries;
} pr_reg_t;

static u32 g_pr_generation = 0;

#define make_pr_reg(arg_nexus, arg_key) \
    (pr_reg_t){ .rsv_type=0, .scope=0, .key=arg_key, \
    .generation= g_pr_generation++, .vol_id=0, .aptpl=0, .mapped_lun_id = 0, \
    .nexus = arg_nexus, }

pr_reg_t* create_pr_reg(itnexus_t arg_nexus, u64 arg_key){
    LETVAR(pr, (pr_reg_t*)malloc(sizeof(pr_reg_t)));
    bzero(pr, sizeof(pr_reg_t));
    pr->key = arg_key; pr->nexus = arg_nexus;
    pr->generation = g_pr_generation++;
    return pr;
}

typedef struct{
    char str[128];
}pr_reg_str_t;
pr_reg_str_t pr_reg_tostr(const pr_reg_t* pr) { 
    pr_reg_str_t ret;
    snprintf(ret.str, ARRAY_SIZE(ret.str), "{key=0x%016lx, gen=%u}", pr->key, pr->generation);
    return ret;
}

#define DEL (void*)(-1)
#define is2(x,a,b) ((x)==(a) || (x) ==(b))

typedef struct{
    itnexus_t  key;
    pr_reg_t*  value;
}entry_t;

#define key_equal(key1, key2)  (memcmp((key1).b, (key2).b, sizeof((key1).b)) == 0)

enum{
    g_hashtable_cap= 257,
};

static entry_t g_hashtable[g_hashtable_cap];
static u16 g_hashtable_valid=0;
static u16 g_hashtable_deleted=0;

static inline int mod_int(int x, int y){
    assert(y>=2);
    if(x>=0)
        return x%y;

    return (y - (-x)%y) % y ;
}
static inline u16 mod(int x)
{
    int ret = mod_int(x, g_hashtable_cap);
    assert(ret < g_hashtable_cap && ret >=0);
    return (u16)ret;
}
static inline u16 mod_inc(u16 h)
{
   ++h;
   return h != g_hashtable_cap ? h : 0;
}

static inline u32 hash_multiplicative(const u8* key, size_t len, u32 INITIAL_VALUE, u32 M)
{
   u32 hash = INITIAL_VALUE;
   for(size_t i = 0; i < len; ++i)
      hash = M * hash + key[i];
   return hash ;
}
static inline u16 naiive_hash_func(itnexus_t key){
    return (u16)(key.oword % g_hashtable_cap);
}

#define hash_func1(key) \
    (u16)(hash_multiplicative((const u8*)(&(key)), sizeof(key), 5381, 33) \
            % g_hashtable_cap)
#define hash_func(key) (u16)( naiive_hash_func(key) )

/*
 * Open Addressing - Linear Probing
 * http://en.wikipedia.org/wiki/Hash_table#Open_addressing
 * http://opendatastructures.org/versions/edition-0.1e/ods-java/5_2_LinearHashTable_Linear_.html
 * http://www.cs.rmit.edu.au/online/blackboard/chapter/05/documents/contribute/chapter/05/linear-probing.html
 */
entry_t* table_alloc(itnexus_t key)
{
    if( g_hashtable_valid==g_hashtable_cap-1){
        LOGERR("hashtable is full, alloc failed");
        return NULL;
    }

    u16 h = hash_func(key);
    u16 orig_h = h;
    entry_t* e = NULL;
    u16 visited = 0;

    do
    {
        e = &g_hashtable[h];
        if( is2(e->value,NULL,DEL))
            goto found_empty_slot;
        else if( key_equal(e->key,  key) ){
            LOGDBG("repeat alloc same key=%s", itnexus_tostr(key).str);
            return e;
        }
        h = mod(h+1);
    } while( ++visited < g_hashtable_cap &&  h != orig_h );

    goto not_found_empty_slot;

found_empty_slot:
    e->key = key;
    ++g_hashtable_valid;
    LOGDBG("Found empty slot, visited=%u", visited);
    return e;
not_found_empty_slot:
    LOGERR("not found empty slot, visited=%u", visited);
    return NULL;
}

entry_t* table_find(itnexus_t key)
{
    if( g_hashtable_valid==0 ){
        LOGINF("hashtable is empty, find failed");
        return NULL;
    }
    u16 h = hash_func(key);
    u16 orig_h = h;
    entry_t* e = NULL;
    u16 visited = 0;

    LOGTRC("h=%u",h);

    do{
        e = &g_hashtable[h];
        if( !is2(e->value,NULL,DEL) &&  key_equal(e->key, key)){
            LOGTRC("e->value != (NULL or DEL) &&  (e->key == key) found_key");
            goto found_key;
        }

        if( e->value == NULL ){
            LOGTRC("if( !e->value) not_found_key ");
            goto not_found_key;
        }
        assert( e->value == DEL || e->value != NULL );

        // step next
        h = mod( h + 1);
    }while( ++visited < g_hashtable_valid && h != orig_h );

    goto not_found_key;

found_key:
    LOGDBG("key=%s is found at index=%u, visited=%u, g_hashtable_valid=%u", 
            itnexus_tostr(key).str, h, visited, g_hashtable_valid );
    return e;
not_found_key:
    LOGERR("key=%s is not found, visited=%u, g_hashtable_valid=%u", 
            itnexus_tostr(key).str, visited, g_hashtable_valid );
    return NULL;
}



pr_reg_t* table_delete(itnexus_t key)
{
    entry_t* e = table_find(key);
    if(!e){
        LOGERR("key=%s does not exist.", itnexus_tostr(key).str);
        return NULL;
    }
    else if( is2(e->value, NULL,DEL) ){
        LOGERR("e->value is %p for key=%s", itnexus_tostr(key).str, pr_reg_tostr(e->value).str );
        return NULL;
    }
    pr_reg_t* ret = e->value;
    e->value = DEL;
    g_hashtable_deleted+=1;
    g_hashtable_valid-=1;
    u16 h =  (u16)(size_t)(e - &g_hashtable[0]);
    LOGINF("h = %u, ret=%s", h, pr_reg_tostr(ret).str );
    return ret;
    /*
    u16 orig_h = h;
    h = mod(h + 1);
    u16 visited = 0;
    do{
        e = &g_hashtable[ h ];
        ++visited;
        if( e->value == NULL){
            u16 dist = mod_dif(home, h);
            if(dist>=1 && dist <= visited ) {
                entry_t* orig_e = &g_hashtable[orig_h];
                swap(orig_e, e);
            }
            break;
        }
        if( e->value != DEL){
            u16 home = hash_func(e->key);
            u16 dist = mod_dif(home, h);
            if(dist>=1 && dist <= visited ) {
            }
        }
    }while(h!=orig_h);
    return true;
    */
}
/*
int visitor(entry_t* e, void* args)
{
    LETVAR(arg_key, (itnexus_t*)args);
    if(key_equal(e->key, *arg_key)){ 
        LOGINF("key=%s,value='%s'", itnexus_tostr(e->key).str, 
                pr_reg_tostr(e->value).str); 
        return 0;
    }else{
        return 1;
    } 
} 
void table_foreach(int (*func)(entry_t*, void*), void* args)
{
    u16 i=0;
    for(; i<g_hashtable_cap; ++i){
        entry_t* e = &g_hashtable[i];
        if(!is2(e->value,NULL,DEL)){
            int cont = (func)(e, args);
            if(!cont)
                break;
        }
    }
}
*/

double table_load_factor()
{
    return ((double)g_hashtable_valid)/((double)g_hashtable_cap);
}

void table_stats()
{
    LOGDBG("load_factor=%f", table_load_factor());

    for(u16 i=0; i< g_hashtable_cap; ++i){
        entry_t* e = &g_hashtable[i];
        if(e->value == NULL){
            LOGINF("[%u]:{NULL}", i);
        }
        else if(e->value==DEL){
            LOGINF("[%u]:{key=%s, DEL}", i, itnexus_tostr(e->key).str);
        }
        if(is2(e->value,NULL,DEL))
            continue;
        LOGINF("[%u]:{key=%s, value=%s}",
                i, itnexus_tostr(e->key).str, pr_reg_tostr(e->value).str );
    }
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

bool table_assign(itnexus_t key, pr_reg_t* value){
    entry_t* e = table_alloc(key);
    if(!e){
        LOGERR("table_alloc failed");
        return false;
    }
    assert( is2(e->value,DEL,NULL) );
    e->value = value;

    return true;
}

pr_reg_t* table_get(itnexus_t key){
    entry_t* e = table_find(key);
    if(!e){
        LOGERR("table_find %s failed", itnexus_tostr(key).str);
        return NULL;
    }
    assert( e->value != DEL );
    return e->value;
}

SLIST_HEAD(slisthead, pr_reg) g_list = {NULL}, g_listdeleted = {NULL};

void slist_add(pr_reg_t* pr)
{
    SLIST_INSERT_HEAD(&g_list, pr, entries);
}

u16 slist_length()
{
    u16 len = 0;
    pr_reg_t  *np = NULL, *tp=NULL;
    // Forward traversal. 
    SLIST_FOREACH_SAFE(np, &g_list, entries, tp){
        ++len;
    }
    return len;
}
pr_reg_t* slist_get_at_index(u16 index)
{
    u16 len = 0;
    pr_reg_t  *np = NULL, *tp=NULL;
    // Forward traversal. 
    SLIST_FOREACH_SAFE(np, &g_list, entries, tp){
        if(len == index){
            return np;
        }
        ++len;
    }
    LOGERR("out of bound: index=%u, len=%u", index, len);
    return NULL;
}

pr_reg_t* slist_find(itnexus_t nexus)
{
    pr_reg_t  *np = NULL, *tp=NULL;
    // Forward traversal. 
    SLIST_FOREACH_SAFE(np, &g_list, entries, tp){
        if( key_equal(np->nexus, nexus))
            return np;
    }
    return NULL;
}

pr_reg_t* slist_delete(itnexus_t nexus){
    pr_reg_t  *np = NULL, *tp=NULL;
    // Forward traversal. 
    SLIST_FOREACH_SAFE(np, &g_list, entries, tp){
        if( key_equal(np->nexus, nexus))
        {
            SLIST_REMOVE(&g_list, np, pr_reg, entries);
            return np;
        }
    }
    return NULL;
}


static inline u8 u8rand()
{
    return (u8)(random() & 0xFF);
}
static inline u16 u16rand()
{
    u16 r = u8rand();
    r<<=8;
    r |= (u16)u8rand();
    return r;
}
static inline u32 u32rand()
{
    u32 r = u16rand();
    r<<=16;
    r |= u16rand();
    return r;
}

static inline u64 u64rand()
{
    u64 r = u32rand();
    r<<=32;
    r |= u32rand();
    return r;
}
static inline u128 u128rand()
{
    u128 r = u64rand();
    r<<=64;
    r |= u64rand();
    return r;
}
static inline u16 u16randlessthan(u16 x)
{
    return u16rand() % x;
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
    u64 iports[8];
    for(size_t i=0; i< ARRAY_SIZE(iports); ++i){
        iports[i] = 0x2001000e1e09f200ULL + i;
    }
    srandom(time(NULL));

    SLIST_INIT(&g_list);// Initialize the list
    SLIST_INIT(&g_listdeleted);// Initialize the list

    for(size_t t=0; t< ARRAY_SIZE(tports); ++t){
        for(size_t i=0; i< ARRAY_SIZE(iports); ++i){
            u64 rk = u64rand();
            itnexus_t nexus = make_itnexus( iports[i], tports[t] );
            LETVAR( pr, create_pr_reg( nexus, rk ));
            table_assign( nexus,  pr );
            slist_add( pr );
        }
    }

    pr_reg_t  *np = NULL;
    // Positive Test: find those already added
    SLIST_FOREACH(np, &g_list, entries){
        entry_t* e = table_find( np->nexus );
        assert( e && !is2(e->value, NULL, DEL) );
        LOGINF("Positive test: nexus=%s, pr=%s", itnexus_tostr(np->nexus).str, pr_reg_tostr(e->value).str);
        assert( memcmp(e->value, np, sizeof(pr_reg_t))==0 );
    }

    // Negative test
    for(u16 i=0; i< 100; ++i) {
        itnexus_t nexus = make_itnexus( u64rand(), u64rand() );
        if( slist_find( nexus ) == NULL ){
            LOGINF("Negative test [%u], random nexus=%s", 
                    i, itnexus_tostr(nexus).str);
            entry_t* e = table_find( nexus );
            assert( e == NULL );
        }
    }
    
    while (1) {
        // List Deletion.
        u16 len = slist_length();
        if(len==0)
            break;
        u16 idx = u16randlessthan( len );
        LOGINF("slist_length()=%u, random index=%u", len, idx);
        pr_reg_t* pr = slist_get_at_index(idx);
        assert(pr);

        SLIST_INSERT_HEAD(&g_listdeleted, pr, entries);
        SLIST_REMOVE(&g_list, pr, pr_reg, entries);
        table_delete(pr->nexus);

        assert( table_find( pr->nexus ) == NULL );

        {
            pr_reg_t  *np = NULL, *tempnp = NULL;
            u16 i = 0;
            SLIST_FOREACH_SAFE(np, &g_listdeleted, entries, tempnp){
                LOGINF("Negative Test: i=%u, nexus=%s", 
                        i, itnexus_tostr(np->nexus).str);
                assert( table_find( np->nexus ) == NULL );
                ++i;
            }
        }
    }
    table_stats();

    while (!SLIST_EMPTY(&g_listdeleted)) {// List Deletion.
        pr_reg_t* n1 = SLIST_FIRST(&g_listdeleted);
        SLIST_REMOVE_HEAD(&g_listdeleted, entries);
        free(n1);
    }

    /*
    assert( table_find(make_itnexus(0x2001000e1e09f20aULL, 0x0100000e1e116080)) == NULL );
    assert( table_find(make_itnexus(0x0001000e1e09f20aULL, 0x2100000e1e116080)) == NULL );
    (void)table_find(make_itnexus(0x2001000e1e09f20aULL, 0x2100000e1e116080));
    (void)table_find(make_itnexus(0x2001000e1e09f203ULL, 0x2001000e1e09f268));
    (void)table_find(make_itnexus(0x2001000e1e09f205, 0x2001000e1e09f282));
    */

    //itnexus_t arg_key = make_itnexus(0x2001000e1e09f268, 0x2001000e1e09f200);
    //table_foreach( visitor, &arg_key);

    return 0;
}

