#include "hashtable_bucket.h"
#include "hash_func.h"
#include "mod_math.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

u32 g_pr_generation = 0;



entry_t g_hashtable[g_hashtable_cap]={0};
u16 g_hashtable_valid=0;
u16 g_hashtable_deleted=0;
u16 g_hashtable_worstcase=0;

        

/*
 * Open Addressing - Linear Probing
 * http://en.wikipedia.org/wiki/Hash_table#Open_addressing
 * http://opendatastructures.org/versions/edition-0.1e/ods-java/5_2_LinearHashTable_Linear_.html
 * http://www.cs.rmit.edu.au/online/blackboard/chapter/05/documents/contribute/chapter/05/linear-probing.html
 */
entry_t* 
table_alloc(itnexus_t key, pr_reg_t value)
{
    u16 empty_buckets = g_hashtable_cap - g_hashtable_valid;
    if( empty_buckets == 0 ){
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

        if( e->state != VALID )
        {
            if(e->state == DELETED)
                --g_hashtable_deleted;
            e->state = VALID;
            e->pr = value;
            e->entries.sle_next = NULL;
            ++g_hashtable_valid;
            g_hashtable_worstcase = __max(g_hashtable_worstcase, visited);
            LOGDBG("Found empty bucket for key=%s, orig_h=%u, h=%u, "
                    "visited=%u,\n"
                    "g_hashtable_valid=%u, g_hashtable_worstcase=%u",  
                    itnexus_tostr(key), orig_h, h, visited, 
                    g_hashtable_valid, g_hashtable_worstcase);
            return e;
        }
        else if( itnexus_equal(e->pr.nexus, key ) ){
            LOGINF("repeat alloc same key=%s", itnexus_tostr(key));
            return e;
        }
        // step next
        ++visited;
        h = mod(h+1);
    } while( h != orig_h );

    LOGERR("Not found empty bucket for key=%s, visited=%u,\n"
            "g_hashtable_valid=%u, g_hashtable_worstcase=%u", 
            itnexus_tostr(key), visited, 
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
    entry_t* e = NULL;
    u16 visited = 0;

    do{
        e = &g_hashtable[h];
        if( e->state == VALID && itnexus_equal(e->pr.nexus, key)){
            LOGDBG("key=%s is found at index=%u, visited=%u, \n"
                    "g_hashtable_valid=%u, g_hashtable_worstcase=%u", 
                    itnexus_tostr(key), h, visited, 
                    g_hashtable_valid, g_hashtable_worstcase );
            return e;
        }

        // step next
        h = mod( h + 1);
        ++visited;
    }while( visited<=g_hashtable_worstcase); 

    LOGINF("key=%s is not found, visited=%u, \n"
            "g_hashtable_valid=%u, g_hashtable_worstcase=%u", 
            itnexus_tostr(key), visited,
            g_hashtable_valid, g_hashtable_worstcase );
    return NULL;
}

bool table_delete(itnexus_t key)
{
    entry_t* e = table_find(key);
    if(!e){
        LOGINF("key=%s does not exist.", itnexus_tostr(key));
        return false;
    }
    PI_ASSERT(e->state == VALID);

    u16 h =  (u16)(e - g_hashtable);

    PI_ASSERT( h< g_hashtable_cap );

    entry_t* next_e = &g_hashtable[ mod(h+1) ];
    if(next_e->state == NIL){
        e->state = NIL;
        // no need to bridge next_e
    }
    else{
        e->state = DELETED; 
        g_hashtable_deleted+=1;
    }
    g_hashtable_valid-=1;

    return true;
}

void table_clean_deleted()
{
    u16 h = mod(-1);
    for(; h!=0; h = mod(h-1)){
        entry_t* e = &g_hashtable[h];
        entry_t* prev_e = &g_hashtable[mod(h-1)];
        if( e->state == NIL && prev_e->state == DELETED){
            prev_e->state = NIL;
        }
    }
}

void table_make_key_return_home(u16 h)
{
    PI_ASSERT(h<g_hashtable_cap);
    entry_t* e = &g_hashtable[ h ];
    PI_ASSERT(e->state == VALID);

    u16  orig_h = hash_func(e->pr.nexus);
    while( h != orig_h)
    {
        entry_t* prev_e = &g_hashtable[ mod(h-1)];
        if(prev_e->state != VALID){
            SWAP(*prev_e, *e);
        }
        h = mod(h-1);
    }
}


double table_load_factor()
{
    return ((double)g_hashtable_valid)/((double)g_hashtable_cap);
}

#define square(arg_x) ({ typeof(arg_x) x = arg_x; x*x; })

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
        if(e->state == NIL)
            continue;

        u16 orig_h = hash_func(e->pr.nexus); 
        u16 diff_h = mod_dif(h, orig_h);
        if(e->state==DELETED)
            LOGLOG("[%u]:{orig_h=%u, diff_h=%u, key=%s, DELETED}",
                    h, orig_h, diff_h, itnexus_tostr(e->pr.nexus));
        else
            LOGLOG("[%u]:{orig_h=%u, diff_h=%u, key=%s, value=%s}",
                    h, orig_h, diff_h, itnexus_tostr(e->pr.nexus), 
                    pr_reg_tostr(e->pr) );
        
        sum += diff_h;
        worst_case = __max(worst_case, diff_h);
    }
    LOGLOG("worst_case=%u", worst_case);
    PI_ASSERT(worst_case <= g_hashtable_worstcase);


    double mean = (double)(sum)/(double)(g_hashtable_cap);
    LOGLOG("mean=%f", mean);
    // Standard Deviation
    u32 sq_mean=0;
    for(u16 h=0; h< g_hashtable_cap; ++h){
        entry_t* e = &g_hashtable[h];
        if( e->state != VALID )
            continue;
        u16 orig_h = hash_func(e->pr.nexus);
        u16 diff_h = mod_dif(h, orig_h);
        sq_mean += square(diff_h );
    }
    double stddev = sqrt((double)sq_mean/(double)(g_hashtable_cap));
    LOGLOG("stddev=%f", stddev);
}

