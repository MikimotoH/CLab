#include <utility>
#include <algorithm>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

using namespace std;
class mystr{
public:
    int   len;
    char* buf;
    mystr()
        : len(0) , buf(NULL)
    {
        printf("trivia ctor\n");
    }
    mystr(int l)
        : len(0) , buf(NULL)
    {
        printf("length-spec ctor, l=%d\n", l);
        buf = (char*)malloc(len);
        len = l;
        memset(buf, 0, len);
    }
    mystr(const mystr& rhs){
        printf("copy ctor const& \"%s\", len=%d\n", rhs.buf, 
                rhs.len);
        buf = strdup(rhs.buf);
        len = rhs.len;
    }

    ~mystr(){
        if(buf){
            printf("dtor free (\"%s\"), len=%d\n", buf, len);
            free(buf);
            buf = NULL;
            len = 0;
        }
        else{
            printf("dtor nothing to free\n");
        }
    }
    mystr(const char* rhs)
        : len(0), buf(NULL)
    {
        printf("ctor from char* \"%s\"\n", rhs);
        buf = strdup(rhs);
        len = strlen(buf);
    }
    
    mystr(mystr&& rhs)
        : len(0), buf(NULL)
    {
        printf("ctor RRValue; swap: buf=\"%s\" len=%d \n", rhs.buf, 
                rhs.len);
        std::swap(buf, rhs.buf);
        std::swap(len, rhs.len);
    }

    mystr& operator=(const mystr& rhs){
        printf("assignment copy: \"%s\"\n", rhs.buf);
        if(buf)
            free(buf);
        buf = strdup(rhs.buf);
        len = rhs.len;
        return *this;
    }

    mystr& operator+=(const mystr& rhs){
        printf("concat \"%s\" + \"%s\", len=(%d + %d)\n", buf, 
                rhs.buf, len, rhs.len);
        char* newbuf = (char*)malloc(len + rhs.len + 1);
        memcpy(newbuf, buf, len);
        memcpy(newbuf+len, rhs.buf, rhs.len);
        newbuf[len+rhs.len]=0;
        free(buf);
        buf = newbuf;
        len += rhs.len;
        return *this;
    }
};


mystr foo(mystr rhs){
    printf("etner foo\n");
    mystr ret("foo ");
    ret += rhs;
    printf("[foo] return \"%s\"\n", ret.buf);
    return ret;
}
int main(){
    mystr a("apple");
    mystr fooapple = foo( std::move(a) );
    return 0;
}

