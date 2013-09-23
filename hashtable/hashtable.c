#include "hashtable.h"
#include "pi_utils.h"
#include "mod_math.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <sys/queue.h>

static u32 g_pr_generation = 0;


pr_reg_t* create_pr_reg(itnexus_t arg_nexus, u64 arg_key){
    LETVAR(pr, (pr_reg_t*)malloc(sizeof(pr_reg_t)));
    bzero(pr, sizeof(pr_reg_t));
    pr->key = arg_key; pr->nexus = arg_nexus;
    pr->generation = g_pr_generation++;
    return pr;
}



entry_t g_hashtable[g_hashtable_cap]={0};
u16 g_hashtable_valid=0;
u16 g_hashtable_deleted=0;
u16 g_hashtable_worstcase=0;

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
entry_t* table_alloc(itnexus_t key, pr_reg_t* value)
{
    u16 empty_buckets = g_hashtable_cap - g_hashtable_valid;
    if( empty_buckets == 0 ){
        LOGERR("hashtable is full, alloc failed");
        return NULL;
    }

    u16 h = hash_func(key);
    u16 orig_h = h;
    entry_t* e = NULL;
    u16 visited_valid = 0;

    do
    {
        e = &g_hashtable[h];
        if( is2(e->value,NULL,DELETED))
            goto found_empty_bucket;
        else if( itnexus_equal(e->key,  key) ){
            LOGINF("repeat alloc same key=%s", itnexus_tostr(key));
            return e;
        }
        // step next
        ++visited_valid;
        h = mod(h+1);
    } while( h != orig_h );

    goto not_found_empty_bucket;

found_empty_bucket:
    e->key = key;
    ++g_hashtable_valid;
    if(e->value == DELETED)
        --g_hashtable_deleted;
    e->value = value;
    g_hashtable_worstcase = __max(g_hashtable_worstcase, visited_valid);
    LOGDBG("Found empty bucket for key=%s, orig_h=%u, h=%u, visited_valid=%u,\n"
            "g_hashtable_valid=%u, g_hashtable_worstcase=%u",  
            itnexus_tostr(key), orig_h, h, visited_valid, 
            g_hashtable_valid, g_hashtable_worstcase);
    return e;

not_found_empty_bucket:
    LOGERR("Not found empty bucket for key=%s, visited_valid=%u,\n"
            "g_hashtable_valid=%u, g_hashtable_worstcase=%u", 
            itnexus_tostr(key), visited_valid, 
            g_hashtable_valid, g_hashtable_worstcase);
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
    u16 visited_valid = 0;

    LOGTRC("h=%u",h);

    do{
        e = &g_hashtable[h];
        if( !is2(e->value,NULL,DELETED) &&  itnexus_equal(e->key, key)){
            LOGTRC("e->value != (NULL or DELETED) &&  (e->key == key) found_key");
            goto found_key;
        }

        if( e->value == NULL ){
            LOGTRC("if( !e->value) not_found_key ");
            goto not_found_key;
        }
        PI_ASSERT( e->value == DELETED || e->value != NULL );

        // step next
        h = mod( h + 1);
        if(e->value != DELETED)
            ++visited_valid;
    }while( visited_valid<=g_hashtable_worstcase); // h != orig_h );

    goto not_found_key;

found_key:
    LOGDBG("key=%s is found at index=%u, visited_valid=%u, \n"
            "g_hashtable_valid=%u, g_hashtable_worstcase=%u", 
            itnexus_tostr(key), h, visited_valid, 
            g_hashtable_valid, g_hashtable_worstcase );
    return e;
not_found_key:
    LOGINF("key=%s is not found, visited_valid=%u, \n"
            "g_hashtable_valid=%u, g_hashtable_worstcase=%u", 
            itnexus_tostr(key), visited_valid,
            g_hashtable_valid, g_hashtable_worstcase );
    return NULL;
}

pr_reg_t* table_delete(itnexus_t key)
{
    entry_t* e = table_find(key);
    if(!e){
        LOGINF("key=%s does not exist.", itnexus_tostr(key));
        return NULL;
    }
    else if( is2(e->value, NULL,DELETED) ){
        LOGERR("e->value is %p for key=%s", 
                itnexus_tostr(key), pr_reg_tostr(e->value) );
        PI_ASSERT(0);
        return NULL;
    }
    pr_reg_t* ret = e->value;
    u16 h =  (u16)(size_t)(e - &g_hashtable[0]);
    LOGTRC("h = %u, ret=%s", h, pr_reg_tostr(ret));

    entry_t* next_e = &g_hashtable[ mod(h+1) ];
    if(next_e->value == NULL){
        e->value = NULL;
        // no need to bridge next_e
    }
    else{
        e->value = DELETED; 
        g_hashtable_deleted+=1;
    }
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
    PI_ASSERT(h<g_hashtable_cap);
    entry_t* e = &g_hashtable[ h ];
    PI_ASSERT(!is2(e->value, NULL, DELETED));

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


double table_load_factor()
{
    return ((double)g_hashtable_valid)/((double)g_hashtable_cap);
}

static inline int square(int x)
{
    return x*x;
}
void table_stats()
{
    LOGLOG("g_hashtable_cap=%u, g_hashtable_valid=%u, g_hashtable_deleted=%u, \n"
            "g_hashtable_worstcase=%u, load_factor=%f", 
            g_hashtable_cap, g_hashtable_valid, g_hashtable_deleted, 
            g_hashtable_worstcase, table_load_factor());

    u16 worst_case = 0;
    u32 sum=0;

    for(u16 h=0; h< g_hashtable_cap; ++h){
        entry_t* e = &g_hashtable[h];
        if(e->value == NULL)
            continue;
        u16 orig_h = hash_func(e->key);
        u16 diff_h = mod_dif(h, orig_h);
        LOGLOG("[%u]:{orig_h=%u, diff_h=%u, key=%s, value=%s}",
                h, orig_h, diff_h, itnexus_tostr(e->key), 
                (e->value==DELETED)?"DELETED":pr_reg_tostr(e->value) );
        if(e->value != DELETED){
            sum += diff_h;
            worst_case = __max(worst_case, diff_h);
        }
    }
    LOGLOG("worst_case=%u", worst_case);
    double mean = (double)(sum)/(double)(g_hashtable_cap);
    LOGLOG("mean=%f", mean);
    // Standard Deviation
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


/*
bool table_assign(itnexus_t key, pr_reg_t* value){
    entry_t* e = table_alloc(key, value);
    if(!e){
        LOGERR("table_alloc failed");
        return false;
    }
    PI_ASSERT( is2(e->value,DELETED,NULL) );
    e->value = value;

    return true;
}
*/

