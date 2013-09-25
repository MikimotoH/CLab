#pragma once

#include "pi_utils.h"
#include <sys/cdefs.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <sys/queue.h>


#define MAX_WWPN_BUF_LEN 8
typedef union{
    u8 b[MAX_WWPN_BUF_LEN];
    uint64_t qword;
} __packed wwpn_t;
_Static_assert(sizeof(wwpn_t)==MAX_WWPN_BUF_LEN,"");

static inline wwpn_t __overloadable 
make_wwpn(u64 qw)
{
   return  (wwpn_t){.qword=qw};
}
static inline wwpn_t __overloadable 
make_wwpn(const u8* b)
{
    wwpn_t r;
    __builtin_memcpy(r.b, b, 8);
    return r;
}
static inline bool __overloadable
wwpn_equal(wwpn_t port1, wwpn_t port2)
{
    return port1.qword == port2.qword;
}

static inline bool __overloadable
wwpn_equal(const u8* b1, const u8* b2)
{
    return __builtin_memcmp(b1,b2,8)==0;
}

/*
 * http://tools.ietf.org/html/rfc3720#section-3.2.6.1
 * iSCSI: maximum length is 223 bytes
 * 
 *                  Naming     String defined by
 *     Type  Date    Auth      "example.com" naming authority
 *    +--++-----+ +---------+ +--------------------------------+
 *    |  ||     | |         | |                                | *    iqn.2001-04.com.example:storage:diskarrays-sn-a8675309
 */
#define MAX_IQN_BUF_LEN 224
typedef struct 
{
    char b[MAX_IQN_BUF_LEN];//plus one for Null-terminator
} __packed iqn_t;
_Static_assert(sizeof(iqn_t)==MAX_IQN_BUF_LEN, "");

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
        union{
            struct{
                wwpn_t i;
                wwpn_t t;
            } __packed;
            u128  oword;//octal words
        } fcp;
        struct{
            iqn_t i;
            iqn_t t;
        } __packed iscsi;
    };
} itnexus_t;



static inline itnexus_t __overloadable
make_fcp_itnexus(wwpn_t iport, wwpn_t tport)
{
    return (itnexus_t){ .protid = PROTID_FCP,
        .fcp.i = iport, .fcp.t = tport };
}
static inline itnexus_t __overloadable
make_fcp_itnexus(u64 iport, u64 tport)
{
    return (itnexus_t){ .protid = PROTID_FCP,
        .fcp.i = make_wwpn(iport), .fcp.t= make_wwpn(tport) };
}

static inline itnexus_t __overloadable
make_fcp_itnexus(const u8* iport, const u8* tport)
{
    return (itnexus_t){ .protid = PROTID_FCP,
        .fcp.i = make_wwpn(iport), .fcp.t= make_wwpn(tport) };
}

static inline iqn_t 
make_iqn(const char* str)
{
    iqn_t r;
    BZERO(r);
    strlcpy(r.b, str, ARRAY_SIZE(r.b));
    return r;
}

static inline bool 
iqn_equal(iqn_t port1, iqn_t port2)
{
    return __builtin_strcmp(port1.b, port2.b);
}


static inline itnexus_t __overloadable
make_iscsi_itnexus(iqn_t iport, iqn_t tport)
{
    return (itnexus_t){
        .protid = PROTID_ISCSI, 
        .iscsi.i= (iport), 
        .iscsi.t= (tport)
    };
}

static inline itnexus_t __overloadable
make_iscsi_itnexus(const char* iport, const char* tport)
{
    return (itnexus_t){
        .protid = PROTID_ISCSI, 
        .iscsi.i= make_iqn(iport), 
        .iscsi.t= make_iqn(tport),
    };
}

static inline bool  
itnexus_equal(itnexus_t n1, itnexus_t n2)
{
    _Static_assert(sizeof(n1.fcp) ==sizeof(u128), "");
    if(n1.protid == PROTID_FCP && n1.protid == n2.protid)
        return n1.fcp.oword == n2.fcp.oword ;
    if(n1.protid == PROTID_ISCSI && n1.protid == n2.protid)
        return iqn_equal(n1.iscsi.i, n2.iscsi.i)
            && iqn_equal(n1.iscsi.t, n2.iscsi.t) ;
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
