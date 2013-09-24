#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <machine/_types.h>
#include <sys/types.h>
#include <machine/cpufunc.h>

#define LOGERR(fmtstr, ...)  \
    printf("<ERROR>[%s]" fmtstr "\n", __func__, ## __VA_ARGS__)

#define LOGINF(fmtstr, ...)  \
    printf("<INFO>[%s]" fmtstr "\n", __func__, ## __VA_ARGS__)

#define LOGDBG(fmtstr, ...)  \
    printf("<DEBUG>[%s][%u]" fmtstr "\n", __func__, __LINE__, ## __VA_ARGS__)

#define LOGTRC(fmtstr, ...)  \
    printf("<TRACE>[%s][%u]" fmtstr "\n", __func__, __LINE__, ## __VA_ARGS__)

#define LOGLOG(fmtstr, ...)  \
    printf(fmtstr "\n", ## __VA_ARGS__)

#define ARRAY_SIZE(x)   (sizeof(x) / sizeof((x)[0]))
#define typeof(x)                       __typeof(x)
#define LETVAR(newvar,rhs)              typeof(rhs) newvar = (rhs) 
#define SWAP(x,y) ( {__typeof__(x) z = x ; x = y; y = z;}  )

#define __max(x, y) ({				\
	typeof(x) _max1 = (x);			\
	typeof(y) _max2 = (y);			\
	(void) (&_max1 == &_max2);		\
	_max1 > _max2 ? _max1 : _max2; })

#define	PI_ASSERT(exp) \
    do {\
        if (__predict_false(!(exp))){                  \
            printf("[%s:%u] PI_ASSERT(%s) failed!\n",  \
                    __func__, __LINE__, # exp );        \
            breakpoint();\
            exit(EXIT_FAILURE);\
        }\
    } while (0)

#define	PI_VERIFY(exp) \
    do {\
        if (__predict_false(!(exp))){                  \
            printf("[%s:%u] PI_VERIFY(%s) failed!\n",  \
                    __func__, __LINE__, # exp );        \
            exit(EXIT_FAILURE);\
        }\
    } while (0)

#define pod_dup(pod) \
    ({ \
        LETVAR(dup, (typeof(pod)*)malloc( sizeof(typeof(pod))));\
        dup ? (memcpy(dup, &pod, sizeof(typeof(pod))), dup)\
            : NULL;\
     })
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wincompatible-pointer-types-discards-qualifiers"
#define pod_ptr_dup(pod_ptr) \
    ({ \
        LETVAR(dup_ptr, (typeof(pod_ptr))malloc( sizeof(typeof( *pod_ptr))));\
        dup_ptr ? (memcpy(dup_ptr, pod_ptr, sizeof(typeof(*pod_ptr))), dup_ptr)\
            : NULL;\
     })
#pragma GCC diagnostic pop

#define BZERO(x) bzero(&x, sizeof(x)) 

