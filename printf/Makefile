SRC=printf.c

CXX=~/.vim/bin/cc_args.py clang
LD=gcc
CXXFLAGS = -I. -std=c99 -g  -v
TARGET=test

all: 
	$(CXX)  $(CXXFLAGS) $(SRC) -o $(TARGET) 


.PHONY: clean run
clean:
	rm -rf *.o $(TARGET)
run:
	./$(TARGET)

