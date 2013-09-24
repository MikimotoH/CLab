#pragma once
#include "itnexus.h"

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

extern u32 g_pr_generation;

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

typedef enum{
    NIL = 0,
    DELETED = ~(0),
    VALID = 1,
}entry_state_e;

typedef struct entry{
    pr_reg_t   pr;
    entry_state_e state;

    // test for SLIST_HEAD
    SLIST_ENTRY(entry) entries;
}entry_t;


extern entry_t* table_alloc(itnexus_t key, pr_reg_t value);

extern entry_t* table_find(itnexus_t key);

extern bool table_delete(itnexus_t key);

extern void table_stats();

extern double table_load_factor();

enum
{
    g_hashtable_cap= 257,
};

extern entry_t g_hashtable[g_hashtable_cap];
extern u16 g_hashtable_valid;
extern u16 g_hashtable_deleted;
extern u16 g_hashtable_worstcase;



