CC=clang
KMOD=vnode_test	
SRCS=vnode_test.c	
SRCS+= vnode_if.h

.include <bsd.kmod.mk>

# replace language from iso-C99 to gnu C11
#CFLAGS := $(CFLAGS:-std\=iso9899:1999=)
#CFLAGS := $(CFLAGS:-fformat-extensions=)
#CFLAGS := $(CFLAGS:-O2=)
CFLAGS += -g -std=gnu11 -O0 -fno-omit-frame-pointer
