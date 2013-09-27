#pragma once
#include "itnexus.h"
#include "hash_func.h"
#include "mod_math.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

typedef struct pr_reg 
{
    itnexus_t nexus; // unique key
    uint64_t rsvkey;
    uint32_t generation;
    uint8_t  scope : 4;
    uint8_t  type  : 4;
    uint8_t  aptpl     : 1;
    uint8_t  rsvr1     : 1; // not used
    uint8_t  all_tg_pt : 1;
    uint8_t  rsvr5     : 5; // not used
    u32      mapped_lun_id;
} pr_reg_t;

static u32 g_pr_generation = 0;

static inline pr_reg_t 
make_pr_reg(itnexus_t nexus, u64 rsvkey)
{
    return (pr_reg_t){
        .nexus = nexus,
        .rsvkey=rsvkey, 
        .generation= g_pr_generation++
    };
}


extern pr_reg_t* create_pr_reg(itnexus_t arg_nexus, u64 arg_key);

#define pr_reg_tostr(pr) \
    ({char str[128];\
     snprintf(str, sizeof(str),  \
         "{key=0x%016lx, gen=%u}", (pr).rsvkey, (pr).generation);\
     str;})

typedef enum entry_state {
    NONE = (u8)(0),
    DELETED = (u8)(~(0)),
    VALID = (u8)(1),
} __packed entry_state_e;
_Static_assert(sizeof(entry_state_e) == sizeof(u8), "");

typedef struct entry{
    pr_reg_t   pr;
    entry_state_e state;

    // test for SLIST_HEAD
    //SLIST_ENTRY(entry) entries;
}entry_t;

enum
{
    g_hashtable_cap= 257,
};

static entry_t g_hashtable[g_hashtable_cap];
static u16 g_hashtable_valid=0;
static u16 g_hashtable_deleted=0;
static u16 g_hashtable_worstcase=0;


static inline u16 
hash_func(itnexus_t nexus)
{
    u32 h=5381;
    if(nexus.protid == PROTID_FCP){
        h = hash_byte_array( (u8*)&nexus.fcp, sizeof(nexus.fcp), 
                h);
    }
    else if(nexus.protid == PROTID_ISCSI){
        h = hash_string(nexus.iscsi.i.b, h);
        h = hash_string(nexus.iscsi.t.b, h);
    }
    return (u16)(h % g_hashtable_cap);
}

static inline u16 mod(int x)
{
    int ret = int32mod(x, g_hashtable_cap);
    PI_ASSERT(ret < g_hashtable_cap && ret >=0);
    return (u16)ret;
}

static inline u16 mod_inc(u16 h)
{
   ++h;
   return h != g_hashtable_cap ? h : 0;
}

/*
 * @return (a - b) mod n
 * if mod_number==257 ;then
 *   mod_dif(0, 256) == 1
 *   mod_dif(256, 0) == 256
 *   mod_dif(256, 255) == 1
 *   mod_dif(255, 256) == 256
 *   mod_dif(1, 0) == 1
 *   mod_dif(0, 1) == 256
 *   mod_dif(-1, 255) == 1
 *   mod_dif(255, -1) == 256
 *   mod_dif(257, 256) == 1
 *   mod_dif(256, 257) == 256
 *   mod_dif(0, -1) == 1
 *   mod_dif(-1, 0) == 256
*/
static inline int32_t int32mod_dif(int32_t a, int32_t b)
{
    int32_t r = int32mod(a-b, g_hashtable_cap);
    PI_ASSERT(r>=0 && r<g_hashtable_cap);
    return r;
}

static inline u16 mod_dif(u16 a, u16 b)
{
    return (u16)int32mod_dif((int32_t)a, (int32_t)b);
}

/*
 * Open Addressing - Linear Probing
 * http://en.wikipedia.org/wiki/Hash_table#Open_addressing
 * http://opendatastructures.org/versions/edition-0.1e/ods-java/5_2_LinearHashTable_Linear_.html
 * http://www.cs.rmit.edu.au/online/blackboard/chapter/05/documents/contribute/chapter/05/linear-probing.html
 */
static inline entry_t* 
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
            LOGINF("repeat alloc same key =%s, e->pr.nexus=%s", 
                    itnexus_tostr(key), itnexus_tostr(e->pr.nexus));
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

static inline entry_t* 
table_find(itnexus_t key)
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
        else if( e->state == NONE)
            break;

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

static inline bool 
table_delete(itnexus_t key)
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
    if(next_e->state == NONE){
        e->state = NONE;
        // no need to bridge next_e
    }
    else{
        e->state = DELETED; 
        g_hashtable_deleted+=1;
    }
    g_hashtable_valid-=1;

    return true;
}

static inline pr_reg_t* 
table_getreg(itnexus_t key)
{
    entry_t* e = table_find(key);
    if(!e)
        return NULL;
    PI_ASSERT(e->state == VALID);
    return &e->pr;
}

static inline entry_t* 
iter_begin()
{
    entry_t* it = &g_hashtable[0];
    for(; it != &g_hashtable[g_hashtable_cap]; ++it){
        if(it->state == VALID)
            return it;
    }
    return it;
}
static inline bool 
iter_end(entry_t* it)
{
    return it == &g_hashtable[g_hashtable_cap];
}
static inline entry_t* 
iter_inc(entry_t* it)
{
    for(; it != &g_hashtable[g_hashtable_cap]; ++it){
        if(it->state == VALID){
            return it;
        }
    }
    return it;
}
#define table_foreach(it) \
    for(entry_t* it = iter_begin(); !iter_end(it); it=iter_inc(it))

static inline void 
table_clean_deleted()
{
    u16 h = mod(-1);
    /* TODO XXX FIXME */
    for(; h!=0; h = mod(h-1)){
        entry_t* e = &g_hashtable[h];
        entry_t* prev_e = &g_hashtable[mod(h-1)];
        if( e->state == NONE && prev_e->state == DELETED){
            prev_e->state = NONE;
        }
    }
}

static inline void 
table_make_key_return_home(u16 h)
{
    PI_ASSERT(h<g_hashtable_cap);
    entry_t* e = &g_hashtable[ h ];
    PI_ASSERT(e->state == VALID);
    /* TODO XXX FIXME */

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


static inline double 
table_load_factor()
{
    return ((double)g_hashtable_valid)/((double)g_hashtable_cap);
}

#define square(arg_x) ({ typeof(arg_x) x = arg_x; x*x; })

static inline void 
table_stats()
{
    LOGLOG("g_hashtable_cap=%u, g_hashtable_valid=%u, g_hashtable_deleted=%u, \n"
            "g_hashtable_worstcase=%u, load_factor=%f", 
            g_hashtable_cap, g_hashtable_valid, g_hashtable_deleted, 
            g_hashtable_worstcase, table_load_factor());

    u16 worst_case = 0;
    u16 num_deleted = 0;
    u16 num_valid = 0;
    u16 num_none  = 0;
    u32 sum=0;

    for(u16 h=0; h< g_hashtable_cap; ++h){
        entry_t* e = &g_hashtable[h];
        if(e->state == NONE){
            ++num_none;
            continue;
        }

        u16 orig_h = hash_func(e->pr.nexus); 
        u16 diff_h = mod_dif(h, orig_h);
        if(e->state==DELETED){
            ++num_deleted;
            LOGLOG("[%u]:{orig_h=%u, diff_h=%u, key=%s, DELETED}",
                    h, orig_h, diff_h, itnexus_tostr(e->pr.nexus));
        }
        else if (e->state == VALID) {
            ++num_valid;
            LOGLOG("[%u]:{orig_h=%u, diff_h=%u, key=%s, value=%s}",
                    h, orig_h, diff_h, itnexus_tostr(e->pr.nexus), 
                    pr_reg_tostr(e->pr) );
        }
        else{
            LOGERR("invalid e->state = %08x", e->state);
            PI_ASSERT(0);
        }
        sum += diff_h;
        worst_case = __max(worst_case, diff_h);
    }
    LOGLOG("worst_case=%u", worst_case);
    PI_ASSERT(worst_case <= g_hashtable_worstcase);
    PI_ASSERT(num_deleted == g_hashtable_deleted);
    PI_ASSERT(num_valid == g_hashtable_valid);
    PI_ASSERT((num_deleted + num_valid + num_none) == g_hashtable_cap );

    u16 denom = num_deleted + num_valid ;
    if(denom != 0){
        double mean = (double)(sum)/denom;
        LOGLOG("mean=%f", mean);
        PI_ASSERT( mean < (double)worst_case);
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
        double stddev = sqrt((double)sq_mean/denom);
        LOGLOG("stddev=%f", stddev);
    }
}

