#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>
#include <time.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
extern inline __attribute__((__gnu_inline__))
int sumv (int a, ...);

inline __attribute__((__gnu_inline__)) 
int sumv (int a, ...) 
{
    int argc = __builtin_va_arg_pack_len ();
    printf("sumv1, argc=%d\n", argc);
    int sum=a;
    va_list arglist;
    va_start(arglist, a);
    while(argc){
        int p = va_arg(arglist, int);
        sum += p;
        argc-=4;
    }
    va_end(arglist);
    return sum;
}

inline __attribute__((__gnu_inline__)) 
int sumv (int a, ...) 
{
    int argc = 8;//__builtin_va_arg_pack_len ();
    printf("sumv2, argc=%d\n", argc);
    int sum=a;
    va_list arglist;
    va_start(arglist, a);
    while(argc){
        int p = va_arg(arglist, int);
        printf("sumv2: p=%d\n", p);
        sum += p;
        argc-=4;
    }
    va_end(arglist);
    return sum;
}

