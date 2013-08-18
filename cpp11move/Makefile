SRC=cpp11move.cpp

CC=g++49

CFLAGS = -I. -I/usr/local/lib/gcc49/include/c++ -std=c++11 -g  
TARGET=test

all:
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

.PHONY: clean run
clean:
	rm -rf *.o $(TARGET)
run:
	./$(TARGET)

