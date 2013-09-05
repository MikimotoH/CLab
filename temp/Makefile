SRC=temp.cpp

CXX=~/.vim/bin/cc_args.py clang++
LD=clang++
CXXFLAGS = -I. -std=c++11 -g  -stdlib=libc++ -v -lstdc++ -l:libcxxrt.a
#CXXFLAGS+= -v 
TARGET=test



all: 
	$(CXX)  $(CXXFLAGS) $(SRC) -o $(TARGET) 


.PHONY: clean run
clean:
	rm -rf *.o $(TARGET)
run:
	./$(TARGET)

