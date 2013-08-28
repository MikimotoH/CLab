#include <iostream>
#include <string>

using namespace std;

int main(int argc, char** argv){
    std::string str = {"hello world"};
    std::cout << str << endl;
    if(argc>=2){
        string b = argv[1];
        cout << "b=" << b <<endl;
    }
    

    return 0;
}
