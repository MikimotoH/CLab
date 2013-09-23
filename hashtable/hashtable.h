#pragma once
#include <sys/cdefs.h>
#include <stdint.h>
#include <sys/queue.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef unsigned __int128 u128;

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

#define make_itnexus(iport, tport) (itnexus_t){.i.qword=iport, .t.qword=tport}
#define itnexus_equal(key1, key2)  (key1.oword == key2.oword) 

#define itnexus_tostr(key) \
    ({\
        char str[16*2 + 6 + 2+2]; \
        snprintf(str, sizeof(str), "\"%016lx <=> %016lx\"", key.i.qword, key.t.qword);\
        str;\
    })

typedef struct pr_reg {
    itnexus_t nexus;
    u64 key;
    u8 rsv_type : 4;
    u8 scope    : 4;
    u8  aptpl;
    u16 vol_id;
    u32 generation;
    // test for SLIST_HEAD
    SLIST_ENTRY(pr_reg) entries;
} pr_reg_t;



extern pr_reg_t* create_pr_reg(itnexus_t arg_nexus, u64 arg_key);

#define pr_reg_tostr(pr) \
    ({char str[128];\
     snprintf(str, sizeof(str),  "{key=0x%016lx, gen=%u}", pr->key, pr->generation);\
     str;})

typedef struct {
    itnexus_t  key;
    pr_reg_t*  value;
}entry_t;

extern entry_t* table_alloc(itnexus_t key, pr_reg_t* value);

extern entry_t* table_find(itnexus_t key);

extern pr_reg_t* table_delete(itnexus_t key);

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

#define DELETED (void*)(-1)

#define is2(x,a,b) ((x)==(a) || (x) ==(b))
