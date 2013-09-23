#include "hashtable.h"
#include "pi_utils.h"
#include <time.h>
#include <string.h>

/*
pr_reg_t* table_get(itnexus_t key){
    entry_t* e = table_find(key);
    if(!e){
        LOGERR("table_find %s failed", itnexus_tostr(key));
        return NULL;
    }
    PI_ASSERT( e->value != DELETED );
    return e->value;
}
*/

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
        if( itnexus_equal(np->nexus, nexus))
            return np;
    }
    return NULL;
}

pr_reg_t* slist_delete(itnexus_t nexus){
    pr_reg_t  *np = NULL, *tp=NULL;
    // Forward traversal. 
    SLIST_FOREACH_SAFE(np, &g_list, entries, tp){
        if( itnexus_equal(np->nexus, nexus))
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
    
    u64 iports[24];
    for(size_t i=0; i< ARRAY_SIZE(iports); ++i){
        iports[i] = 0x2001000e1e09f200ULL + i;
    }
    srandom(time(NULL));

    SLIST_INIT(&g_list);// Initialize the list
    SLIST_INIT(&g_listdeleted);// Initialize the list

    PI_ASSERT(slist_length()==0);
    u16 expected_total_count = ARRAY_SIZE(tports) * ARRAY_SIZE(iports);

    for(size_t t=0; t< ARRAY_SIZE(tports); ++t){
        for(size_t i=0; i< ARRAY_SIZE(iports); ++i){
            u64 rk = u64rand();
            itnexus_t nexus = make_itnexus( iports[i], tports[t] );
            LETVAR( pr, create_pr_reg( nexus, rk ));
            PI_VERIFY(pr);
            PI_VERIFY(table_alloc( nexus,  pr ) != NULL );
            slist_add( pr );
        }
    }
    LOGINF("slist_length=%u, g_hashtable_valid=%u", 
            slist_length(), g_hashtable_valid);
    PI_ASSERT(slist_length() == expected_total_count);
    PI_ASSERT(g_hashtable_valid == expected_total_count);
    PI_ASSERT( slist_length() == g_hashtable_valid );

    pr_reg_t  *np = NULL;
    
    // Positive Test: find those already added
    SLIST_FOREACH(np, &g_list, entries){
        entry_t* e = table_find( np->nexus );
        PI_ASSERT( e && !is2(e->value, NULL, DELETED) );
        LOGINF("Positive test: nexus=%s, pr=%s", 
                itnexus_tostr(e->value->nexus), pr_reg_tostr(e->value));
        PI_ASSERT( memcmp(e->value, np, sizeof(pr_reg_t))==0 );
    }

    // Random Negative test
    for(u16 i=0; i< 1000; ++i) {
        itnexus_t nexus = make_itnexus( u64rand(), u64rand() );
        if( slist_find( nexus ) == NULL ){
            LOGINF("Random Negative Test [%u], random nexus=%s", 
                    i, itnexus_tostr(nexus));
            PI_VERIFY( table_find( nexus ) == NULL );
        }
    }
   
    
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
        PI_ASSERT(pr);
        itnexus_t key = pr->nexus;
        SLIST_REMOVE(&g_list, pr, pr_reg, entries);
        ++deleted;

        pr->entries.sle_next = NULL;
        SLIST_INSERT_HEAD(&g_listdeleted, pr, entries);

        if(table_delete(key) == NULL){
            LOGERR("table_delete(%s) failed.", itnexus_tostr(key));
            TABLE_STATS("After deleted key=%s", itnexus_tostr(key) );
            breakpoint();
        }

        PI_ASSERT( table_find(key) == NULL );

        TABLE_STATS("After deleted key=%s", itnexus_tostr(key) );

        {
            pr_reg_t  *np = NULL, *tempnp = NULL;
            u16 i = 0;
            SLIST_FOREACH_SAFE(np, &g_listdeleted, entries, tempnp){
                LOGINF("Negative Test: i=%u, nexus=%s", 
                        i, itnexus_tostr(np->nexus));
                PI_ASSERT( table_find( np->nexus ) == NULL );
                ++i;
            }
        }
    }
    LOGINF("deleted=%u, expected_total_count=%u", deleted, expected_total_count);
    PI_ASSERT( deleted == expected_total_count);

    LOGINF("g_hashtable_valid=%u, g_hashtable_deleted=%u", 
            g_hashtable_valid, g_hashtable_deleted);
    PI_ASSERT( g_hashtable_valid == 0);
    TABLE_STATS("After fully deleted");

    while (!SLIST_EMPTY(&g_listdeleted)) {// List Deletion.
        pr_reg_t* n1 = SLIST_FIRST(&g_listdeleted);
        SLIST_REMOVE_HEAD(&g_listdeleted, entries);
        free(n1);
    }
    LOGINF("return OK");

    /*
    PI_ASSERT( table_find(make_itnexus(0x2001000e1e09f20aULL, 0x0100000e1e116080)) == NULL );
    PI_ASSERT( table_find(make_itnexus(0x0001000e1e09f20aULL, 0x2100000e1e116080)) == NULL );
    (void)table_find(make_itnexus(0x2001000e1e09f20aULL, 0x2100000e1e116080));
    (void)table_find(make_itnexus(0x2001000e1e09f203ULL, 0x2001000e1e09f268));
    (void)table_find(make_itnexus(0x2001000e1e09f205, 0x2001000e1e09f282));
    */

    //itnexus_t arg_key = make_itnexus(0x2001000e1e09f268, 0x2001000e1e09f200);
    //table_foreach( visitor, &arg_key);

    return 0;
}

