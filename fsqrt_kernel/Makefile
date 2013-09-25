#CC=gcc49
CC=clang
KMOD=sqrt	
SRCS=sqrt.c	

.PHONY: clean
clean:
	rm -rf x86 machine @ RCS 
	rm -f $(KMOD).ko  export_syms *.o

.include <bsd.kmod.mk>

# replace language from iso-C99 to gnu C11
CFLAGS := $(CFLAGS:-std\=iso9899:1999=)
CFLAGS := $(CFLAGS:-fformat-extensions=)
CFLAGS := $(CFLAGS:-mno-mmx=)
CFLAGS := $(CFLAGS:-mno-sse=)
CFLAGS := $(CFLAGS:-msoft-float=)
CFLAGS += -g -std=gnu11 

