#include "hashtable.h"
#include "pi_utils.h"
#include "mod_math.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

u32 g_pr_generation = 0;


entry_t* create_entry(const itnexus_t* arg_nexus, const pr_reg_t* pr){
    LETVAR(ent, (entry_t*)malloc(sizeof(entry_t)));
    PI_VERIFY(ent);
    bzero(ent, sizeof(entry_t));
    ent->pr = *pr; 
    return ent;
}


entry_t* g_hashtable[g_hashtable_cap]={0};
u16 g_hashtable_valid=0;
u16 g_hashtable_deleted=0;
u16 g_hashtable_worstcase=0;

static inline u32 hash_multiplicative(const u8* key, size_t len, 
        u32 INITIAL_VALUE, u32 M)
{
   u32 hash = INITIAL_VALUE;
   for(size_t i = 0; i < len; ++i)
      hash = M * hash + key[i];
   return hash ;
}
static inline u32
hash_string(const char *str)
{
    u32 hash = 5381;
    char c;

    while ((c = *str++) != 0)
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}

static inline u16 hash_func(const itnexus_t* nexus)
{
    u32 h=0;
    if(nexus->protid == PROTID_FCP){
        h = hash_multiplicative( (u8*)&nexus->fcp, sizeof(nexus->fcp), 
                5381, 33);
    }
    else if(nexus->protid == PROTID_ISCSI){
        u16 ilen = strlen(nexus->iscsi.i.b);
        u16 tlen = strlen(nexus->iscsi.t.b);
        PI_ASSERT(ilen>4 && tlen>4);

        char str[ilen-4 + tlen-4 + 1];
        memcpy(str       , nexus->iscsi.i.b+4, ilen-4);
        memcpy(str+ilen-4, nexus->iscsi.t.b+4, tlen-4);
        str[ilen-4+tlen-4] = 0;
        h = hash_string(str);
    }
    return (u16)(h % g_hashtable_cap);
}
        


/*
#define hash_func1(key) \
    (u16)(hash_multiplicative((const u8*)(&(key)), sizeof(key), 5381, 33) \
            % g_hashtable_cap)
*/
//#define hash_func(key) (u16)( naiive_hash_func(key) )
//#define hash_func hash_func1


/*
 * Open Addressing - Linear Probing
 * http://en.wikipedia.org/wiki/Hash_table#Open_addressing
 * http://opendatastructures.org/versions/edition-0.1e/ods-java/5_2_LinearHashTable_Linear_.html
 * http://www.cs.rmit.edu.au/online/blackboard/chapter/05/documents/contribute/chapter/05/linear-probing.html
 */
entry_t** 
table_alloc(entry_t* e)
{
    PI_ASSERT( e );
    u16 empty_buckets = g_hashtable_cap - g_hashtable_valid;
    if( empty_buckets == 0 ){
        LOGERR("hashtable is full, alloc failed");
        return NULL;
    }

    const itnexus_t* key = &e->pr.nexus;
    u16 h = hash_func(&e->pr.nexus);
    u16 orig_h = h;
    entry_t** pe = NULL;
    u16 visited_valid = 0;

    do
    {
        pe = &g_hashtable[h];
        if( is2(*pe, NIL, DELETED))
            goto found_empty_bucket;
        else if( itnexus_equal((*pe)->pr.nexus, *key ) ){
            LOGINF("repeat alloc same key=%s", itnexus_tostr(*key));
            return pe;
        }
        // step next
        ++visited_valid;
        h = mod(h+1);
    } while( h != orig_h );

    goto not_found_empty_bucket;

found_empty_bucket:
    if(*pe == DELETED)
        --g_hashtable_deleted;
    *pe = e; 
    ++g_hashtable_valid;
    g_hashtable_worstcase = __max(g_hashtable_worstcase, visited_valid);
    LOGDBG("Found empty bucket for key=%s, orig_h=%u, h=%u, "
            "visited_valid=%u,\n"
            "g_hashtable_valid=%u, g_hashtable_worstcase=%u",  
            itnexus_tostr(*key), orig_h, h, visited_valid, 
            g_hashtable_valid, g_hashtable_worstcase);
    return pe;

not_found_empty_bucket:
    LOGERR("Not found empty bucket for key=%s, visited_valid=%u,\n"
            "g_hashtable_valid=%u, g_hashtable_worstcase=%u", 
            itnexus_tostr(*key), visited_valid, 
            g_hashtable_valid, g_hashtable_worstcase);
    return NULL;
}

entry_t** table_find(const itnexus_t* key)
{
    PI_ASSERT(key);
    if( g_hashtable_valid==0 ){
        LOGINF("hashtable is empty, find failed");
        return NULL;
    }
    u16 h = hash_func(key);
    u16 orig_h = h;
    entry_t** pe = NULL;
    u16 visited = 0;

    LOGTRC("h=%u",h);

    do{
        pe = &g_hashtable[h];
        if( !is2(*pe,NIL,DELETED) &&  itnexus_equal((*pe)->pr.nexus, *key)){
            LOGTRC("pe->value != (NIL or DELETED) &&  (pe->key == key) found_key");
            goto found_key;
        }

        // step next
        h = mod( h + 1);
        ++visited;
    }while( visited<=g_hashtable_worstcase); // h != orig_h );

    goto not_found_key;

found_key:
    LOGDBG("key=%s is found at index=%u, visited=%u, \n"
            "g_hashtable_valid=%u, g_hashtable_worstcase=%u", 
            itnexus_tostr(*key), h, visited, 
            g_hashtable_valid, g_hashtable_worstcase );
    return pe;

not_found_key:
    LOGINF("key=%s is not found, visited=%u, \n"
            "g_hashtable_valid=%u, g_hashtable_worstcase=%u", 
            itnexus_tostr(*key), visited,
            g_hashtable_valid, g_hashtable_worstcase );
    return NULL;
}

bool table_delete(const itnexus_t* key)
{
    PI_ASSERT(key);

    entry_t** pe = table_find(key);
    if(!pe){
        LOGINF("key=%s does not exist.", itnexus_tostr(*key));
        return false;
    }

    pr_reg_t* pr = &(*pe)->pr;
    u16 h =  (u16)(size_t)(pe - &g_hashtable[0]);
    LOGTRC("h = %u, pr=%s", h, pr_reg_tostr(*pr));

    entry_t** next_pe = &g_hashtable[ mod(h+1) ];
    if(*next_pe== NIL){
        *pe = NIL;
        // no need to bridge next_pe
    }
    else{
        *pe = DELETED; 
        g_hashtable_deleted+=1;
    }
    g_hashtable_valid-=1;

    return true;
}

void table_clean_deleted()
{
    u16 h = mod(-1);
    for(; h!=0; h = mod(h-1)){
        entry_t** pe = &g_hashtable[h];
        entry_t** prev_pe = &g_hashtable[mod(h-1)];
        if( *pe == NIL && *prev_pe == DELETED){
            *prev_pe= NIL;
        }
    }
}

void table_make_key_return_home(u16 h)
{
    PI_ASSERT(h<g_hashtable_cap);
    entry_t** pe = &g_hashtable[ h ];
    PI_ASSERT(!is2(*pe, NIL, DELETED));

    u16  home_h = hash_func(&(*pe)->pr.nexus);
    while( h != home_h)
    {
        entry_t** prev_pe = &g_hashtable[ mod(h-1)];
        if(is2(*prev_pe, NIL, DELETED)){
            SWAP(*prev_pe, *pe);
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
        entry_t** pe = &g_hashtable[h];
        if(is2(*pe, NIL, DELETED))
            continue;

        pr_reg_t* pr = &(*pe)->pr;
        itnexus_t key = pr->nexus; 
        u16 orig_h = hash_func(&key); 
        u16 diff_h = mod_dif(h, orig_h);
        LOGLOG("[%u]:{orig_h=%u, diff_h=%u, key=%s, value=%s}",
                h, orig_h, diff_h, itnexus_tostr(key), 
                pr_reg_tostr(*pr) );
        sum += diff_h;
        worst_case = __max(worst_case, diff_h);
    }
    LOGLOG("worst_case=%u", worst_case);
    double mean = (double)(sum)/(double)(g_hashtable_cap);
    LOGLOG("mean=%f", mean);
    // Standard Deviation
    u32 sq_mean=0;
    for(u16 h=0; h< g_hashtable_cap; ++h){
        entry_t** pe = &g_hashtable[h];
        if( is2(*pe, NIL,DELETED) )
            continue;
        pr_reg_t* pr = &(*pe)->pr;
        itnexus_t key = pr->nexus; 
        u16 orig_h = hash_func(&key);
        u16 diff_h = mod_dif(h, orig_h);
        sq_mean += square(diff_h );
    }
    double stddev = sqrt((double)sq_mean/(double)(g_hashtable_cap));
    LOGLOG("stddev=%f", stddev);
}

