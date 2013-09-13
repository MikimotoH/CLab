#SRC=dump_hex_line.c
#shuffle.c
SRC=shuffle.c

CC=gcc49

#CFLAGS= -I. -fno-omit-frame-pointer  -fno-asynchronous-unwind-tables -ffreestanding -fstack-protector -std=gnu11 -fstack-protector -Wstrict-prototypes  -Wno-pointer-sign -fdiagnostics-show-option -g

CFLAGS = -I. -std=gnu11 -g 
TARGET=test

all:
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

.PHONY: clean run
clean:
	rm -rf *.o $(TARGET)
run:
	./$(TARGET)

