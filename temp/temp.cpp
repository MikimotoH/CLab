#include <iostream>
#include <string>
#include <stdio.h>
#include <stdlib.h>

using namespace std;
/**
 * http://en.wikipedia.org/wiki/Modular_arithmetic
 */
int mod(int x, int y){
    if(y<0)
        return (exit(-1), printf("denominator can't be negative"), -1);
    if(x>=0)
        return x%y;

    return y - (-x)%y;
}

int main(int argc, char** argv){
    if(argc != 3)
        return EINVAL;

    int x = atoi(argv[1]);
    int y = atoi(argv[2]);
    if(y<0)
        return (printf("y=%d can NOT be negative",y), EINVAL);
    int z = mod(x,y);

    printf("mod(%d,%d)=%d", x,y,z);
    return 0;
}

    
int main1(int argc, char** argv){
    std::string str = {"hello world"};
    std::cout << str << endl;
    if(argc>=2){
        string b = argv[1];
        cout << "b=" << b <<endl;
    }
    return 0;
}
