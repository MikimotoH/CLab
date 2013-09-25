#include "hashtable_bucket.h"
#include "pi_utils.h"
#include "bitmap.h"
#include <errno.h>

#define VEC_ELEM_TYPE  pr_reg_t
#include "vector.h"
#include "unittest_vector.h"

#define VEC_ELEM_TYPE  pr_reg_t

static inline VEC_ELEM_TYPE*
vector_find(const VECTOR_T* self, itnexus_t nexus)
{
    for(u16 i=0; i<self->size; ++i){
        if( itnexus_equal(self->data[i].nexus, nexus) )
            return &self->data[i];
    }
    return NULL;
}

VECTOR_T g_vector;
VECTOR_T g_vectordeleted;
#undef VEC_ELEM_TYPE

#define TABLE_STATS(label, ...) \
        printf( label "\n"\
                "-------------------------------------------------\n", ## __VA_ARGS__ ); \
        table_stats();\
        printf( "-------------------------------------------------\n");

int main(int argc, char** argv){
    //unittest_int32mod_dif();
    unittest_bitmap();
    unittest_vector();
    LOGINF("sizeof(g_hashtable)=%lu", sizeof(g_hashtable));


    u32 num_fcp_tports   =0;
    u32 num_iscsi_tports =0;
    u32 num_fcp_iports   =0;
    u32 num_iscsi_iports =0;
    if(argc==1) {
        num_fcp_tports   = 8;
        num_iscsi_tports = 8;
        num_fcp_iports   = 8;
        num_iscsi_iports = 8;
    }
    else if(argc==5){
        num_fcp_tports = atoi(argv[1]);
        num_iscsi_tports = atoi(argv[2]);
        num_fcp_iports = atoi(argv[3]);
        num_iscsi_iports = atoi(argv[4]);
    }
    else{
        printf("Usage: \n" 
                "\t %s <num_fcp_tports> <num_iscsi_tports> <num_fcp_iports> <num_iscsi_tports>\n", 
                argv[0]);
        return EINVAL;
    }


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

    vector_init(&g_vector, g_hashtable_cap);
    vector_init(&g_vectordeleted, g_hashtable_cap);

    PI_ASSERT(vector_length(&g_vector)==0);
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
            vector_append(&g_vector, ent->pr);
            LOGINF("vector_length()=%u", vector_length(&g_vector));
        }
    }
    for(u32 t=0; t< num_iscsi_tports; ++t){
        for(u32 i=0; i< num_iscsi_iports; ++i){
            u64 rk = u64rand();
            itnexus_t nexus = make_iscsi_itnexus( iscsi_iports[i], iscsi_tports[t] );
            pr_reg_t pr = make_pr_reg( nexus, rk );
            entry_t* ent=NULL;
            PI_VERIFY((ent=table_alloc(nexus, pr)) != NULL );
            vector_append(&g_vector, ent->pr );
            LOGINF("vector_length()=%u", vector_length(&g_vector));
        }
    }

    LOGINF("vector_length=%u, g_hashtable_valid=%u", 
            vector_length(&g_vector), g_hashtable_valid);
    PI_ASSERT( vector_length(&g_vector) == expected_total_count );
    PI_ASSERT( g_hashtable_valid == expected_total_count );
    PI_ASSERT( vector_length(&g_vector) == g_hashtable_valid );

    
    // Positive Test: find those already added

    vector_foreach(np, g_vector)
    {
        entry_t* ent = table_find( np->nexus );
        if( !ent ){
            TABLE_STATS("table_find(key=%s) failed.", itnexus_tostr(np->nexus) );
            exit(EXIT_FAILURE);
        }
        LOGINF("Positive test: nexus=%s, pr=%s", 
                itnexus_tostr( ent->pr.nexus ), pr_reg_tostr( ent->pr));
        PI_ASSERT( memcmp( &ent->pr, np, sizeof(pr_reg_t))==0 );
    }

    // Random Negative test
    for(u16 i=0; i< 10; ) {
        itnexus_t nexus = make_fcp_itnexus( u64rand(), u64rand() );
        if( vector_find(&g_vector, nexus ) == NULL ){
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
        if( vector_find( &g_vector, nexus ) == NULL ){
            LOGINF("Random Negative Test [%u], random nexus=%s", 
                    i, itnexus_tostr(nexus));
            PI_VERIFY( table_find( nexus ) == NULL );
            ++i;
        }
    }
   
    
    TABLE_STATS("After fully loaded.");

    u16 deleted = 0;
    while ( !vector_is_empty(&g_vector)) 
    {
        // List Deletion.
        u16 len = vector_length(&g_vector);
        if(len==0)
            break;
        u16 idx = u16rand() % len;
        LOGINF("vector_length()=%u, random index=%u", len, idx);
        pr_reg_t pr = *vector_get_at_index(&g_vector, idx);
        itnexus_t nexus = pr.nexus;
        vector_delete_at_index(&g_vector, idx);
        ++deleted;

        vector_append(&g_vectordeleted, pr );

        if(table_delete(nexus) == false){
            LOGERR("table_delete(%s) failed.", itnexus_tostr(nexus));
            TABLE_STATS("After deleted key=%s", itnexus_tostr(nexus) );
            breakpoint();
        }

        PI_ASSERT( table_find(nexus) == NULL );

        TABLE_STATS("After deleted key=%s", itnexus_tostr(nexus) );

        {
            u16 i = 0;
            vector_foreach(np, g_vectordeleted)
            {
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

    vector_cleanup(&g_vectordeleted);
    vector_cleanup(&g_vector);
    LOGINF("return OK");
    return 0;
}

