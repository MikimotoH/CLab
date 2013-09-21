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
#include <math.h>
#include <sys/queue.h>
#include <machine/_types.h>
#include <sys/types.h>
#include <machine/cpufunc.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef unsigned __int128 u128;
#define LOGERR(fmtstr, ...)  \
    printf("<ERROR>[%s]" fmtstr "\n", __func__, ## __VA_ARGS__)

#define LOGINF(fmtstr, ...)  \
    printf("<INFO>[%s]" fmtstr "\n", __func__, ## __VA_ARGS__)

#define LOGDBG(fmtstr, ...)  \
    printf("<DEBUG>[%s][%u]" fmtstr "\n", __func__, __LINE__, ## __VA_ARGS__)

#define LOGTRC(fmtstr, ...)  \
    printf("<TRACE>[%s][%u]" fmtstr "\n", __func__, __LINE__, ## __VA_ARGS__)

#define LOGLOG(fmtstr, ...)  \
    printf(fmtstr "\n", ## __VA_ARGS__)

#define ARRAY_SIZE(x)   (sizeof(x) / sizeof((x)[0]))
#define typeof(x)                       __typeof(x)
#define LETVAR(newvar,rhs)              typeof(rhs) newvar = (rhs) 
#define SWAP(x,y) ( {__typeof__(x) z = x ; x = y; y = z;}  )

#define __max(x, y) ({				\
	typeof(x) _max1 = (x);			\
	typeof(y) _max2 = (y);			\
	(void) (&_max1 == &_max2);		\
	_max1 > _max2 ? _max1 : _max2; })

#define _ASSERT(expr) \
    do{if(!(expr)){\
        printf("[%s:%u]assert(%s) failed\n", __FILE__, __LINE__, # expr );\
        breakpoint();\
    }}while(0)

static inline int square(int x)
{
    return x*x;
}
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
}pr_reg_tostr_t;

pr_reg_tostr_t pr_reg_tostr(const pr_reg_t* pr) 
{
    pr_reg_tostr_t ret;
    snprintf(ret.str, ARRAY_SIZE(ret.str), "{key=0x%016lx, gen=%u}", pr->key, pr->generation);
    return ret;
}

#define DELETED (void*)(-1)
#define is2(x,a,b) ((x)==(a) || (x) ==(b))

typedef struct{
    itnexus_t  key;
    pr_reg_t*  value;
}entry_t;

#define key_equal(key1, key2)  (memcmp((key1).b, (key2).b, sizeof((key1).b)) == 0)

enum
{
    g_hashtable_cap= 257,
};

static entry_t g_hashtable[g_hashtable_cap];
static u16 g_hashtable_valid=0;
static u16 g_hashtable_deleted=0;

static inline int32_t int32mod(int32_t x, int32_t y)
{
    assert(y>=2);
    if(x>=0)
        return x%y;

    int32_t r =  (y - (-x)%y) % y ;
    assert(r>=0 && r<y);
    return r;
}

static inline u16 mod(int x)
{
    int ret = int32mod(x, g_hashtable_cap);
    assert(ret < g_hashtable_cap && ret >=0);
    return (u16)ret;
}

static inline u16 mod_inc(u16 h)
{
   ++h;
   return h != g_hashtable_cap ? h : 0;
}

// @return (a - b) mod n
// if mod_number==257 ;then
//   mod_dif(0, 256) == 1
//   mod_dif(256, 0) == 256
//   mod_dif(256, 255) == 1
//   mod_dif(255, 256) == 256
//   mod_dif(1, 0) == 1
//   mod_dif(0, 1) == 256
//   mod_dif(-1, 255) == 1
//   mod_dif(255, -1) == 256
//   mod_dif(257, 256) == 1
//   mod_dif(256, 257) == 256
//   mod_dif(0, -1) == 1
//   mod_dif(-1, 0) == 256
static inline int32_t int32mod_dif(int32_t a, int32_t b)
{
    int32_t r = int32mod(a-b, g_hashtable_cap);
    assert(r>=0 && r<g_hashtable_cap);
    return r;
}

static inline u16 mod_dif(u16 a, u16 b)
{
    return (u16)int32mod_dif((int32_t)a, (int32_t)b);
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
//#define hash_func(key) (u16)( naiive_hash_func(key) )
#define hash_func hash_func1

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
        if( is2(e->value,NULL,DELETED))
            goto found_empty_bucket;
        else if( key_equal(e->key,  key) ){
            LOGDBG("repeat alloc same key=%s", itnexus_tostr(key).str);
            return e;
        }
        h = mod(h+1);
    } while( ++visited < g_hashtable_cap &&  h != orig_h );

    goto not_found_empty_bucket;

found_empty_bucket:
    e->key = key;
    ++g_hashtable_valid;
    LOGDBG("Found empty bucket for key=%s, orig_h=%u, h=%u, visited=%u,\n"
            "g_hashtable_valid=%u",  
            itnexus_tostr(key).str, orig_h, h, 
            visited, g_hashtable_valid);
    return e;

not_found_empty_bucket:
    LOGERR("Not found empty bucket for key=%s, visited=%u,\n"
            "g_hashtable_valid=%u", 
            itnexus_tostr(key).str, visited, 
            g_hashtable_valid);
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
        if( !is2(e->value,NULL,DELETED) &&  key_equal(e->key, key)){
            LOGTRC("e->value != (NULL or DELETED) &&  (e->key == key) found_key");
            goto found_key;
        }

        if( e->value == NULL ){
            LOGTRC("if( !e->value) not_found_key ");
            goto not_found_key;
        }
        assert( e->value == DELETED || e->value != NULL );

        // step next
        h = mod( h + 1);
    }while( h != orig_h );

    goto not_found_key;

found_key:
    LOGDBG("key=%s is found at index=%u, visited=%u, g_hashtable_valid=%u", 
            itnexus_tostr(key).str, h, visited, g_hashtable_valid );
    return e;
not_found_key:
    LOGINF("key=%s is not found, visited=%u, g_hashtable_valid=%u", 
            itnexus_tostr(key).str, visited, g_hashtable_valid );
    return NULL;
}

#define BZERO(pod) bzero(&pod, sizeof(pod))

pr_reg_t* table_delete(itnexus_t key)
{
    entry_t* e = table_find(key);
    if(!e){
        LOGINF("key=%s does not exist.", itnexus_tostr(key).str);
        return NULL;
    }
    else if( is2(e->value, NULL,DELETED) ){
        LOGERR("e->value is %p for key=%s", itnexus_tostr(key).str, pr_reg_tostr(e->value).str );
        assert(0);
        return NULL;
    }
    pr_reg_t* ret = e->value;
    u16 h =  (u16)(size_t)(e - &g_hashtable[0]);
    LOGTRC("h = %u, ret=%s", h, pr_reg_tostr(ret).str );

    entry_t* next_e = &g_hashtable[ mod(h+1) ];
    if(next_e->value == NULL)
        e->value = NULL;
    else 
        e->value = DELETED;

    g_hashtable_deleted+=1;
    g_hashtable_valid-=1;

    return ret;
}

void table_clean_deleted()
{
    u16 h = mod(-1);
    for(; h!=0; h = mod(h-1)){
        entry_t* e = &g_hashtable[h];
        entry_t* prev_e = &g_hashtable[mod(h-1)];
        if( e->value == NULL && prev_e->value == DELETED){
            prev_e->value = NULL;
        }
    }
}


void table_make_key_return_home(u16 h)
{
    assert(h<g_hashtable_cap);
    entry_t* e = &g_hashtable[ h ];
    assert(!is2(e->value, NULL, DELETED));

    u16  home_h = hash_func(e->key);
    while( h != home_h)
    {
        entry_t* prev_e = &g_hashtable[ mod(h-1)];
        if(is2(prev_e->value, NULL, DELETED)){
            SWAP(prev_e,e);
        }
        h = mod(h-1);
    }
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
        if(!is2(e->value,NULL,DELETED)){
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
    LOGLOG("g_hashtable_cap=%u, g_hashtable_valid=%u, g_hashtable_deleted=%u, \n"
            "load_factor=%f", 
            g_hashtable_cap, g_hashtable_valid, g_hashtable_deleted, 
            table_load_factor());

    u16 worst_case = 0;
    u32 sum=0;

    for(u16 h=0; h< g_hashtable_cap; ++h){
        entry_t* e = &g_hashtable[h];
        if(e->value == NULL)
            continue;
        u16 orig_h = hash_func(e->key);
        u16 diff_h = mod_dif(h, orig_h);
        LOGLOG("[%u]:{orig_h=%u, diff_h=%u, key=%s, value=%s}",
                h, orig_h, diff_h, itnexus_tostr(e->key).str, 
                (e->value==DELETED)?"DELETED":pr_reg_tostr(e->value).str );
        if(e->value != DELETED){
            sum += diff_h;
            worst_case = __max(worst_case, diff_h);
        }
    }
    LOGLOG("worst_case=%u", worst_case);
    double mean = (double)(sum)/(double)(g_hashtable_cap);
    LOGLOG("mean=%f", mean);
    u32 sq_mean=0;
    for(u16 h=0; h< g_hashtable_cap; ++h){
        entry_t* e = &g_hashtable[h];
        if( is2(e->value,NULL,DELETED) )
            continue;
        u16 orig_h = hash_func(e->key);
        u16 diff_h = mod_dif(h, orig_h);
        sq_mean += square(diff_h );
    }
    double stddev = sqrt((double)sq_mean/(double)(g_hashtable_cap));
    LOGLOG("stddev=%f", stddev);
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
    assert( is2(e->value,DELETED,NULL) );
    e->value = value;

    return true;
}

pr_reg_t* table_get(itnexus_t key){
    entry_t* e = table_find(key);
    if(!e){
        LOGERR("table_find %s failed", itnexus_tostr(key).str);
        return NULL;
    }
    assert( e->value != DELETED );
    return e->value;
}

SLIST_HEAD(slisthead, pr_reg) g_list = {NULL}, g_listdeleted = {NULL};

void slist_add(pr_reg_t* pr)
{
    SLIST_INSERT_HEAD(&g_list, pr, entries);
}

u16 slist_length()
{
    if(SLIST_EMPTY(&g_list))
        return 0;
    pr_reg_t  *np = NULL, *tp=NULL;
    u16 len = 0;
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
    if(x==1)
        return 0;
    return u16rand() % x;
}

void unittest_int32mod_dif()
{
   assert(int32mod_dif(0, 256) == 1);
   assert(int32mod_dif(256, 0) == 256);
   assert(int32mod_dif(256, 255) == 1);
   assert(int32mod_dif(255, 256) == 256);
   assert(int32mod_dif(1, 0) == 1);
   assert(int32mod_dif(0, 1) == 256);
   assert(int32mod_dif(-1, 255) == 1);
   assert(int32mod_dif(255, -1) == 256);
   assert(int32mod_dif(257, 256) == 1);
   assert(int32mod_dif(256, 257) == 256);
   assert(int32mod_dif(0, -1) == 1);
   assert(int32mod_dif(-1, 0) == 256);
}


int main(int argc, char** argv){
    //unittest_int32mod_dif();

    u64 tports[8];
    for(size_t i=0; i< ARRAY_SIZE(tports); ++i){
        tports[i] = 0x2100000e1e116080ULL + i;
    }
    //{ 0x2001000e1e09f268ULL, 
    //    0x2001000e1e09f269ULL, 
    //    0x2001000e1e09f278ULL, 
    //    0x2001000e1e09f279ULL, 
    //    0x2100000e1e116080ULL, 
    //    0x2100000e1e116081ULL, 
    //    0x2001000e1e09f282ULL, 
    //    0x2001000e1e09f283ULL };
    u64 iports[28];
    for(size_t i=0; i< ARRAY_SIZE(iports); ++i){
        iports[i] = 0x2001000e1e09f200ULL + i;
    }
    srandom(time(NULL));

    SLIST_INIT(&g_list);// Initialize the list
    SLIST_INIT(&g_listdeleted);// Initialize the list

    assert(slist_length()==0);
    u16 expected_total_count = ARRAY_SIZE(tports) * ARRAY_SIZE(iports);
    for(size_t t=0; t< ARRAY_SIZE(tports); ++t){
        for(size_t i=0; i< ARRAY_SIZE(iports); ++i){
            u64 rk = u64rand();
            itnexus_t nexus = make_itnexus( iports[i], tports[t] );
            LETVAR( pr, create_pr_reg( nexus, rk ));
            table_assign( nexus,  pr );
            slist_add( pr );
        }
    }
    LOGINF("slist_length=%u, g_hashtable_valid=%u", slist_length(), g_hashtable_valid);
    assert(slist_length() == expected_total_count);
    assert(g_hashtable_valid == expected_total_count);
    assert( slist_length() == g_hashtable_valid );

    pr_reg_t  *np = NULL;
    /*
    // Positive Test: find those already added
    SLIST_FOREACH(np, &g_list, entries){
        entry_t* e = table_find( np->nexus );
        assert( e && !is2(e->value, NULL, DELETED) );
        LOGINF("Positive test: nexus=%s, pr=%s", itnexus_tostr(np->nexus).str, pr_reg_tostr(e->value).str);
        assert( memcmp(e->value, np, sizeof(pr_reg_t))==0 );
    }

    // Random Negative test
    for(u16 i=0; i< 5; ++i) {
        itnexus_t nexus = make_itnexus( u64rand(), u64rand() );
        if( slist_find( nexus ) == NULL ){
            LOGINF("Negative test [%u], random nexus=%s", 
                    i, itnexus_tostr(nexus).str);
            entry_t* e = table_find( nexus );
            assert( e == NULL );
        }
    }
    */
    
#define TABLE_STATS(label, ...) \
        printf( label "\n"\
                "-------------------------------------------------\n", ## __VA_ARGS__ ); \
        table_stats();\
        printf( "-------------------------------------------------\n");
    TABLE_STATS("After fully loaded.");

    u16 deleted = 0;
    while ( !SLIST_EMPTY(&g_list)) 
    {
        // List Deletion.
        u16 len = slist_length();
        if(len==0)
            break;
        u16 idx = len/2; // u16randlessthan( len );
        LOGINF("slist_length()=%u, random index=%u", len, idx);
        pr_reg_t* pr = slist_get_at_index(idx);
        assert(pr);
        itnexus_t key = pr->nexus;
        SLIST_REMOVE(&g_list, pr, pr_reg, entries);
        ++deleted;

        pr->entries.sle_next = NULL;
        SLIST_INSERT_HEAD(&g_listdeleted, pr, entries);

        if(table_delete(key) == NULL){
            LOGERR("table_delete(%s) failed.", itnexus_tostr(key).str);
            TABLE_STATS("After deleted key=%s", itnexus_tostr(key).str );
            breakpoint();
        }

        assert( table_find(key) == NULL );

        TABLE_STATS("After deleted key=%s", itnexus_tostr(key).str );

        /*{
            pr_reg_t  *np = NULL, *tempnp = NULL;
            u16 i = 0;
            SLIST_FOREACH_SAFE(np, &g_listdeleted, entries, tempnp){
                LOGINF("Negative Test: i=%u, nexus=%s", 
                        i, itnexus_tostr(np->nexus).str);
                assert( table_find( np->nexus ) == NULL );
                ++i;
            }
        }*/
    }
    LOGINF("deleted=%u, expected_total_count=%u", deleted, expected_total_count);
    assert( deleted == expected_total_count);

    LOGINF("g_hashtable_valid=%u, g_hashtable_deleted=%u", 
            g_hashtable_valid, g_hashtable_deleted);
    assert( g_hashtable_valid == 0);
    TABLE_STATS("After fully deleted");

    while (!SLIST_EMPTY(&g_listdeleted)) {// List Deletion.
        pr_reg_t* n1 = SLIST_FIRST(&g_listdeleted);
        SLIST_REMOVE_HEAD(&g_listdeleted, entries);
        free(n1);
    }
    LOGINF("return OK");

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


