SRC=hashtable.c

CC=~/.vim/bin/cc_args.py clang33
CFLAGS = -I. -std=gnu11 -g -v -fblocks
TARGET=test


all: 
	$(CC)  $(CFLAGS) $(SRC) -o $(TARGET) 


.PHONY: clean run
clean:
	rm -rf *.o $(TARGET)
run:
	./$(TARGET)

