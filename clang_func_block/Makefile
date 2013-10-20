#CC=gcc49
CC=clang
KMOD=clang_func_block	
SRCS=clang_func_block.c	

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
CFLAGS := $(CFLAGS:-O2=)
CFLAGS := $(CFLAGS:-O1=)
CFLAGS += -g -std=gnu11 -O0 -fblocks

