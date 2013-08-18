#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>
#include <time.h>
#include <fcntl.h>
#include <stdarg.h>


void shuffle(uint32_t arr[], size_t size)
{
    inline void swap(int k, int j){
        uint32_t temp = arr[k];
        arr[k] = arr[j];
        arr[j] = temp;
    }
    size_t lasti = size-1;
    for(; lasti!=1; --lasti){
        int k = (int)(random()%(lasti));
        swap(k,lasti);
    }
    if( random()%2==1 )
        swap(0,1);

}


int main(int argc, char** argv){
    //printf( "sum=%d\n", sumv(1,1,1));

    if(argc<2){
        fprintf(stderr, "usage:\n %s [count]\n", 
                argv[0]);
        return EINVAL;
    }
    int count = atoi(argv[1]);
    if(count < 3 ){
        fprintf( stderr, "count(%d) is too small.\n",
                count);
        return EINVAL;
    }
    srandom( time(NULL));
    uint32_t arr[count];
    for(uint32_t i=0; i<count; ++i)
        arr[i] = i;

    shuffle(arr, count);

    printf("arr[%d]={ ", count);
    for(uint32_t i=0; i<count; ++i)
        printf("%u, ", arr[i]);
    printf("}\n");
    return 0;
}


