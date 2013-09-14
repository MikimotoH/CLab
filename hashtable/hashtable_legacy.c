
/**
 * http://en.wikipedia.org/wiki/Modular_arithmetic
 */
int mod(int x, int y){
    assert(y>=2);
    if(x>=0)
        return x%y;

    return y - (-x)%y;
}
u32 modu32(u32 x, u32 y){
    assert(y>=2);
    return x%y;
}
static inline u32 wrapdec(u32 x){
    --x;
    if(x == -1)
        x=g_hashtable_cap-1;
    return x;
}

u32 hash_func_naiive(itnexus_t key)
{
     return key.oword % g_hashtable_cap;
}

u32 hash_mul2(const itnexus_t key, u32 INITIAL_VALUE, u32 M)
{
    u32* dw = (u32*)&key;
    size_t len = sizeof(key)/sizeof(u32);
    u32 hash = INITIAL_VALUE;
    for(size_t i = 0; i < len; ++i)
        hash = M * hash + dw[i];
    return hash ;
}
#define hash_func2(key) \
    (hash_mul2(key, 5381, 33) % g_hashtable_cap)

#define hash_func  hash_func1

int main(){
    table_assign(make_itnexus(0xba00, "tare");
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
