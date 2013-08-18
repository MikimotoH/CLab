#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <time.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdbool.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef unsigned __int128 u128;
typedef struct{
    u32 key;
    char value[256];
    union{
        u32 cluster_dense;
        u32 is_valid;
    };
    u32 cluster_name;
}entry_t;
enum{
    hashtable_cap=257,
};
static entry_t g_hashtable[hashtable_cap];
static int hashtable_valid=0;
static inline u32 wrapinc(u32 x){
    ++x;
    if(x == hashtable_cap)
        x=0;
    return x;
}
static inline u32 wrapdec(u32 x){
    --x;
    if(x == -1)
        x=hashtable_cap-1;
    return x;
}

entry_t* table_alloc(u32 key){
    if( hashtable_valid==hashtable_cap-2){
        printf("hashtable is full, alloc failed\n");
        return NULL;
    }
    u32 h = key%hashtable_cap;
    u32 hbeg = h;
    int visited=0;

    do{
        entry_t* e = &g_hashtable[h];
        if( !e->is_valid ){
            e->key = key;
            e->is_valid = 1;
            e->cluster_name = hbeg;
            return e;
        }
        if(e->key == key){
            printf("repeat alloc same key=%u\n", key);
            return e;
        }
        // cluster occurs. cluster grows
        if(e->cluster_name == hbeg)
            e->cluster_dense++;

        h = wrapinc(h);
        ++visited;
        if(visited == hashtable_valid){
            printf("visited=%d alloc failed\n", visited);
            break;
        }
    }while(h != hbeg );
    printf("h has wheel a round, alloc fail\n");
    return NULL;
}

entry_t* table_find(u32 key){
    if( hashtable_valid==0 ){
        printf("hashtable is empty, find failed\n");
        return NULL;
    }
    u32 h = key%hashtable_cap;
    u32 hbeg = h;
    int visited=0;

    do{
        entry_t* e = &g_hashtable[h];
        if( e->is_valid && e->key == key){
            return e;
        }
        h = wrapinc(h);
        ++visited;
        if(visited == hashtable_valid){
            printf("visited=%d find failed\n", visited);
            break;
        }
    }while(h != hbeg );
    printf("h has wheel a round, find fail\n");
    return NULL;
}

void table_foreach(int (*func)(entry_t*) ){
    int i=0;
    for(; i<hashtable_cap; ++i){
        entry_t* e = &g_hashtable[i];
        if(e->is_valid){
            int cont = (*func)(e);
            if(!cont)
                break;
        }
    }
}
double table_load_factor(){
    return ((double)hashtable_valid)/((double)hashtable_cap);
}

void table_stats(){
    for(int i=0; i< hashtable_cap; ++i){
        entry_t* e = &g_hashtable[i];
        if(!e->is_valid)
            continue;
        printf("[%d]:{cluster_dense=%u, cluster_name=%u, key=%u, value='%s'}\n",
                i, e->cluster_dense, e->cluster_name, e->key, e->value );
    }
}

bool table_assign(u32 key, const char* value){
    entry_t* e = table_alloc(key);
    if(!e){
        printf("table_alloc failed\n");
        return false;
    }

    strlcpy(e->value, value, sizeof(e->value));
    return true;
}
const char* table_get(u32 key){
    entry_t* e = table_find(key);
    if(!e){
        printf("table_find %u failed\n", key);
        return "XXX unable to find XXX";
    }
    return e->value;
}

#define lambda(return_type, function_body) \
    ({ \
     return_type __fn__ function_body \
     __fn__; \
     })

int main(int argc, char** argv){
    int (*max)(int, int) = lambda (int, (int x, int y) { return x > y ? x : y; });
    max(4, 5); // Example
    table_assign(0, "tare");
    table_assign(1, "geolu");
    table_assign(257, "Brzinsky");
    table_assign(257*2, "apostrophe");
    table_assign(257*3, "cyssan");
    table_assign(257*4, "Gesund");
    table_assign(256+257*1, "combustion");
    table_assign(256+257*2, "collapse");
    table_assign(256+257*3, "Don Quixote");
    table_assign(0, "Visarge");
    table_assign(6, "salvus");
    table_assign(7, "Verna");
    table_assign(8, "Grimm's Law");
    table_assign(9, "Grassman's Law");
    table_assign(10, "peregrinus");
    table_assign(11, "pilgrem");
    table_assign(13, "Thur");
    table_assign(14, "Lord");
    table_assign(15, "hlub");
    table_assign(16, "Bethlehem");
    table_assign(17, "Bedlem");
    table_assign(1342, "Geoffrey Chaucer");
    
    int key = 1;
    table_stats();
    table_foreach( lambda (int, (entry_t* e)
        {
            if(e->key==key){ 
                printf("key=%u,value='%s'\n",e->key,e->value); 
                return 0;
            }else{
                return 1;
            } 
        }) );



    return 0;
}
