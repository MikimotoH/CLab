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
        struct{
            wwpn_t i;
            wwpn_t t;
        } __packed fcp;
        struct{
            iqn_t i;
            iqn_t t;
        } __packed iscsi;
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
