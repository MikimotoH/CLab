#pragma once
#include "pi_utils.h"
#include <sys/cdefs.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
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

#define make_wwpn(qw) (wwpn_t){.qword=qw}

/*
 * http://tools.ietf.org/html/rfc3720#section-3.2.6.1
 * iSCSI: maximum length is 223 bytes
 * 
 *                  Naming     String defined by
 *     Type  Date    Auth      "example.com" naming authority
 *    +--++-----+ +---------+ +--------------------------------+
 *    |  ||     | |         | |                                | *    iqn.2001-04.com.example:storage:diskarrays-sn-a8675309
 */
#define MAX_IQN_BUF_LEN (223+1)
typedef struct 
{
    char b[223+1];//plus one for Null-terminator
} iqn_t;

/*
 * Spc4r17.pdf
 * Table 362 - PROTOCOL IDENTIFIER values
 */
enum{
    PROTID_FCP = 0,
    PROTID_ISCSI=5,
};

typedef struct{
    u32  protid;// PROTID_xxx
    union
    {
        wwpn_t fcp_addr;
        iqn_t  iscsi_addr;
    };
} iniport_t; 
typedef iniport_t tgtport_t;

#define MAX_INIPORT_STR 224
#define MAX_TGTPORT_STR 224

typedef struct{
    u32 protid;
    union{
        struct{
            wwpn_t i;
            wwpn_t t;
        } fcp;
        struct{
            iqn_t i;
            iqn_t t;
        } iscsi;
    };
} itnexus_t;

#define make_fcp_itnexus(iport, tport) \
    (itnexus_t){.protid = PROTID_FCP, \
        .fcp.i=iport, .fcp.t=tport}


static inline iqn_t 
make_iscsiaddr_iqn(const char* str)
{
    iqn_t r;
    BZERO(r);
    strlcpy(r.b, str, ARRAY_SIZE(r.b));
    return r;
}

#define make_iscsi_itnexus(iport, tport) \
    (itnexus_t){\
        .protid = PROTID_ISCSI, \
        .iscsi.i= (iport), \
        .iscsi.t= (tport)\
    }

static inline bool 
itnexus_equal(itnexus_t n1, itnexus_t n2)
{
    if(n1.protid == PROTID_FCP && n1.protid == n2.protid)
        return n1.fcp.i.qword == n2.fcp.i.qword
            && n1.fcp.t.qword == n2.fcp.t.qword;
    if(n1.protid == PROTID_ISCSI && n1.protid == n2.protid)
        return strcmp(n1.iscsi.i.b, n2.iscsi.i.b)==0
            && strcmp(n1.iscsi.t.b, n2.iscsi.t.b)==0 ;
    return false;
}

#define itnexus_tostr(itnexus) \
    ({\
        char str[MAX_INIPORT_STR*2 + 6 + 2+2]; \
        if((itnexus).protid==PROTID_FCP) \
             snprintf(str, sizeof(str), "\"%016lx <=> %016lx\"", \
                (itnexus).fcp.i.qword, (itnexus).fcp.t.qword);       \
        else if((itnexus).protid==PROTID_ISCSI)\
            snprintf(str, sizeof(str), "\"%s <=> %s\"", \
                (itnexus).iscsi.i.b, (itnexus).iscsi.t.b);  \
        str;\
    })

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

typedef struct entry{
    pr_reg_t   pr;

    // test for SLIST_HEAD
    SLIST_ENTRY(entry) entries;
}entry_t;

extern entry_t* create_entry(const itnexus_t* key, const pr_reg_t* value);

extern entry_t** table_alloc(entry_t* e);

extern entry_t** table_find(const itnexus_t* key);

extern bool table_delete(const itnexus_t* key);

extern void table_stats();

extern double table_load_factor();

enum
{
    g_hashtable_cap= 257,
};

extern entry_t* g_hashtable[g_hashtable_cap];
extern u16 g_hashtable_valid;
extern u16 g_hashtable_deleted;
extern u16 g_hashtable_worstcase;

#define DELETED (entry_t*)(-1)
#define NIL     (entry_t*)(0)

#define is2(x,a,b) ((x)==(a) || (x) ==(b))

