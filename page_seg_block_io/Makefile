SRC=page_seg_block_io.c
CC=~/.vim/bin/cc_args.py clang
CFLAGS = -I. -std=gnu11 -g 
TARGET=test


all: 
	$(CC)  $(CFLAGS) $(SRC) -o $(TARGET) 


.PHONY: clean run
clean:
	rm -rf *.o $(TARGET) $(TARGET).core
run:
	./$(TARGET)

