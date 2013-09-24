#include "hashtable_bucket.h"
#include "pi_utils.h"
#include "bitmap.h"
#include <time.h>
#include <string.h>

SLIST_HEAD(slisthead, entry) g_list = {NULL}, g_listdeleted = {NULL};

void slist_add(entry_t* ent)
{
    SLIST_INSERT_HEAD(&g_list, ent, entries);
}

u16 slist_length()
{
    if(SLIST_EMPTY(&g_list))
        return 0;
    entry_t  *np = NULL, *tp=NULL;
    u16 len = 0;
    // Forward traversal. 
    SLIST_FOREACH_SAFE(np, &g_list, entries, tp){
        ++len;
    }
    return len;
}

entry_t* slist_get_at_index(u16 index)
{
    u16 len = 0;
    entry_t  *np = NULL, *tp=NULL;
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

entry_t* slist_find(itnexus_t nexus)
{
    entry_t  *np = NULL, *tp=NULL;
    // Forward traversal. 
    SLIST_FOREACH_SAFE(np, &g_list, entries, tp){
        if( itnexus_equal(np->pr.nexus, nexus))
            return np;
    }
    return NULL;
}

entry_t* slist_delete(itnexus_t nexus){
    entry_t  *np = NULL, *tp=NULL;
    // Forward traversal. 
    SLIST_FOREACH_SAFE(np, &g_list, entries, tp){
        if( itnexus_equal(np->pr.nexus, nexus))
        {
            SLIST_REMOVE(&g_list, np, entry, entries);
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
static inline u32 u32randrng(u32 lower, u32 upper)
{
    u32 diff = upper-lower+1;
    return u32rand()%diff + lower;
}
    

// tports={ 0x2001000e1e09f268ULL, 
//    0x2001000e1e09f269ULL, 
//    0x2001000e1e09f278ULL, 
//    0x2001000e1e09f279ULL, 
//    0x2100000e1e116080ULL, 
//    0x2100000e1e116081ULL, 
//    0x2001000e1e09f282ULL, 
//    0x2001000e1e09f283ULL };
#define TABLE_STATS(label, ...) \
        printf( label "\n"\
                "-------------------------------------------------\n", ## __VA_ARGS__ ); \
        table_stats();\
        printf( "-------------------------------------------------\n");

int main(int argc, char** argv){
    //unittest_int32mod_dif();
    unittest_bitmap();
    LOGINF("sizeof(g_hashtable)=%lu", sizeof(g_hashtable));

    PI_VERIFY(argc==5);

    u32 num_fcp_tports = atoi(argv[1]);
    u32 num_iscsi_tports = atoi(argv[2]);
    u32 num_fcp_iports = atoi(argv[3]);
    u32 num_iscsi_iports = atoi(argv[4]);

    wwpn_t fcp_tports[num_fcp_tports];
    iqn_t iscsi_tports[num_iscsi_tports];
    wwpn_t fcp_iports[num_fcp_iports];
    iqn_t iscsi_iports[num_iscsi_iports];

    for(u32 i=0; i< num_fcp_tports; ++i){
        fcp_tports[i] = make_wwpn(0x2100000e1e116080ULL + i);
    }

    BZERO(iscsi_tports);
    // http://lkml.indiana.edu/hypermail/linux/kernel/0908.0/01082.html
    for(u32 i=0; i<num_iscsi_tports; ++i)
        snprintf(iscsi_tports[i].b, MAX_IQN_BUF_LEN, 
                "iqn.2013-10.com.pizza-core:%02u:2dadf92d0ef", i+1); 

    for(u32 i=0; i< num_fcp_iports; ++i)
        fcp_iports[i] = make_wwpn(0x2001000e1e09f200ULL + i);
    
    BZERO(iscsi_iports);
    for(u32 i=0; i< num_iscsi_iports; ++i)
        snprintf(iscsi_iports[i].b, MAX_IQN_BUF_LEN, 
                "iqn.1996-04.de.suse:%02u:1661f9ee7b5", i+1);

    srandom( rdtsc32() );

    SLIST_INIT(&g_list);// Initialize the list
    SLIST_INIT(&g_listdeleted);// Initialize the list

    PI_ASSERT(slist_length()==0);
    u16 expected_total_count = (num_fcp_iports*num_fcp_tports) 
        + ( num_iscsi_iports * num_iscsi_tports);
    LOGINF("expected_total_count=%u", expected_total_count);

    for(u32 t=0; t< num_fcp_tports; ++t){
        for(u32 i=0; i< num_fcp_iports; ++i){
            u64 rk = u64rand();
            itnexus_t nexus = make_fcp_itnexus( fcp_iports[i], fcp_tports[t] );
            pr_reg_t pr = make_pr_reg( nexus, rk );
            entry_t* ent=NULL;
            PI_VERIFY( (ent = table_alloc(nexus, pr)) != NULL );
            PI_ASSERT(ent->entries.sle_next == NULL);
            slist_add( ent );
            LOGINF("slist_length()=%u", slist_length());
        }
    }
    for(u32 t=0; t< num_iscsi_tports; ++t){
        for(u32 i=0; i< num_iscsi_iports; ++i){
            u64 rk = u64rand();
            itnexus_t nexus = make_iscsi_itnexus( iscsi_iports[i], iscsi_tports[t] );
            pr_reg_t pr = make_pr_reg( nexus, rk );
            entry_t* ent=NULL;
            PI_VERIFY((ent=table_alloc(nexus, pr)) != NULL );
            PI_ASSERT(ent->entries.sle_next == NULL);
            slist_add( ent );
            LOGINF("slist_length()=%u", slist_length());
        }
    }

    LOGINF("slist_length=%u, g_hashtable_valid=%u", 
            slist_length(), g_hashtable_valid);
    PI_ASSERT( slist_length() == expected_total_count );
    PI_ASSERT( g_hashtable_valid == expected_total_count );
    PI_ASSERT( slist_length() == g_hashtable_valid );

    entry_t  *np = NULL;
    
    // Positive Test: find those already added
    SLIST_FOREACH(np, &g_list, entries){
        entry_t* ent = table_find( np->pr.nexus );
        if( !ent ){
            TABLE_STATS("table_find(key=%s) failed.", itnexus_tostr(np->pr.nexus) );
            exit(EXIT_FAILURE);
        }
        LOGINF("Positive test: nexus=%s, pr=%s", 
                itnexus_tostr( ent->pr.nexus ), pr_reg_tostr( ent->pr));
        PI_ASSERT( memcmp( &ent->pr, &np->pr, sizeof(pr_reg_t))==0 );
    }

    // Random Negative test
    for(u16 i=0; i< 10; ) {
        itnexus_t nexus = make_fcp_itnexus( u64rand(), u64rand() );
        if( slist_find( nexus ) == NULL ){
            LOGINF("Random Negative Test [%u], random nexus=%s", 
                    i, itnexus_tostr(nexus));
            PI_VERIFY( table_find( nexus ) == NULL );
            ++i;
        }
    }
    for(u16 i=0; i< 10; ) {
        iqn_t rand_tgt, rand_ini;
        BZERO(rand_tgt); BZERO(rand_ini);
        snprintf(rand_tgt.b, MAX_IQN_BUF_LEN, 
                "iqn.2013-10.com.pizza-core:%02u:%lx", u32randrng(1,12), u64rand()); 
        snprintf(rand_ini.b, MAX_IQN_BUF_LEN, 
                "iqn.1996-04.de.suse:%02u:%lx", u32randrng(1,12), u64rand()); 
        itnexus_t nexus = make_iscsi_itnexus(rand_ini, rand_tgt);
        if( slist_find( nexus ) == NULL ){
            LOGINF("Random Negative Test [%u], random nexus=%s", 
                    i, itnexus_tostr(nexus));
            PI_VERIFY( table_find( nexus ) == NULL );
            ++i;
        }
    }
   
    
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
        entry_t* ent = slist_get_at_index(idx);
        PI_ASSERT(ent);
        itnexus_t nexus = ent->pr.nexus;
        SLIST_REMOVE(&g_list, ent, entry, entries);
        ++deleted;

        ent->entries.sle_next = NULL;
        SLIST_INSERT_HEAD(&g_listdeleted, ent, entries);

        if(table_delete(nexus) == false){
            LOGERR("table_delete(%s) failed.", itnexus_tostr(nexus));
            TABLE_STATS("After deleted key=%s", itnexus_tostr(nexus) );
            breakpoint();
        }

        PI_ASSERT( table_find(nexus) == NULL );

        TABLE_STATS("After deleted key=%s", itnexus_tostr(nexus) );

        {
            entry_t* np = NULL;
            u16 i = 0;
            SLIST_FOREACH(np, &g_listdeleted, entries){
                LOGINF("Negative Test: i=%u, nexus=%s", 
                        i, itnexus_tostr(np->pr.nexus));
                PI_ASSERT( table_find( np->pr.nexus ) == NULL );
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
        entry_t* n1 = SLIST_FIRST(&g_listdeleted);
        SLIST_REMOVE_HEAD(&g_listdeleted, entries);
    }
    LOGINF("return OK");
    return 0;
}

