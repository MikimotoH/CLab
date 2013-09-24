


typedef struct{
    itnexus_t  key;
    pr_reg_t*  value;
    u16 next_node;
    union{
        u16 cluster_name;
        u16 hashvalue;
    };
}entry_t;



int main(int argc, char** argv){
    u64 tports[8]= {
        0x2001000e1e09f268ULL, 
        0x2001000e1e09f269ULL, 
        0x2001000e1e09f278ULL, 
        0x2001000e1e09f279ULL, 
        0x2100000e1e116080ULL, 
        0x2100000e1e116081ULL, 
        0x2001000e1e09f282ULL, 
        0x2001000e1e09f283ULL, 
    };
    return 0;
}

