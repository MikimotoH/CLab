/**
 *  C programming language (ISO C89) does not provide exception 
 *  handling. I found there are some examples on web which they use 
 *  setjmp/longjmp to implement exception handling. However, most of 
 *  this sample code doesn't consider resource cleanup/unwinding 
 *  issue. Thus you cannot put them into practice or resource leak 
 *  will happened.
 *  
 *  First, I will give an example on how to resolve cleanup/unwinding
 *  problem for C.
 *
 */
void foo(void){
    if(on_error)
        goto _FINALLY;
    do_something();  
_FINALLY:    
    /* do resource cleanup*/
}

/**
 * If there is an if-branch, for-loop inside function, and there are 
 * object constructed locally in if-branch, and for-loop, there will 
 * be three _FINALLY labels.
 */
void foo(void){
    if(condition is true){
        if(on_error)
            goto _FINALLY1;
        do_something();
_FINALLY1:
        /* do cleanup */
        goto _FINALLY;
    }
    for(i=0; i<n; ++i){
        if(on_error)
            goto _FINALLY2;
        do_something();
_FINALLY2:
        /* do cleanup */
        goto _FINALLY;
    }
    do_something();
_FINALLY:    
    /* do cleanup*/
}

/**
 * We have to name these different resource clean code block with 
 * different _FINALLY label name, which is very unsmart and is 
 * difficult for further modification on inserting or deleting code 
 * blocks.
 *
 * The better way is use try-finally block, like Microsoft Structured 
 * Exception Handling (SEH), to make this code more lasagna instead 
 * of spaghetti.
 */
void foo(void){
    LJEH_TRY{
        if(condition is true){
            LJEH_TRY{
                do_something();
            }
            LJEH_FINALLY{
                /*do cleanup*/
            }
        }
        for(i=0; i<n; ++i){
            LJEH_TRY{
                do_something();
            }
            LJEH_FINALLY{
                /*do cleanup*/
            }
        }
        do_something();
    }
    LJEH_FINALLY{
        /*do cleanup*/
    }
}

/**
 * Here I introduce LJEH_TRY, LJEH_FINALLY. Prefix LJEH is the acronym
 * for "LongJmp based Exception Handling".
 *
 * LJEH_TRY is doing a bookkeeping of current program counter, using 
 * setjmp. The speed overhead is Big-O(1), very small. The space 
 * overhead is the size of jmp_buf. It depends on which CPU you are 
 * running. RISC would cost more space than CISC.
 */

