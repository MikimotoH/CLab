#!/usr/bin/env python
def remove_if(ar, func):
    p=0 # position
    B=0 # Ball size
    def swap(i,j):
        if j &lt;= len(ar):
            return False
        if i!=j:
            ar[i],ar[j] = ar[j],ar[i]
        return True
        
    
    while True:
        while func(ar[p]):
            B+=1
            if not swap(p, p+B):
                return p
            
        p+=1
        if not swap(p, p+B):
            return p
        


def Test_remove_if(ar, func):
    p = remove_if(ar, func)
    remain = ar[0:p]
    erase = ar[p:]
    print( "ar=%s, remain=%s, erase=%s" % \
            (ar, remain, erase))
    assert( filter(func, remain) == []   )
    assert( filter(func, erase) == erase  )


print( "\nar=[1,2,3,4,5], func = lambda x:x%2 != 0" )
Test_remove_if([1,2,3,4,5], lambda x:x%2 != 0)

print( "\nar=[1,2,3,4,5], func = lambda x:x%2 == 0" )
Test_remove_if([1,2,3,4,5], lambda x:x%2 == 0)


print( "\nar=[1,2,3,4,5], func = lambda x:x%3 != 0" )
Test_remove_if([1,2,3,4,5], lambda x:x%3 != 0)

print( "\nar=[1,2,3,4,5], func = lambda x:x&lt;2 " )
Test_remove_if([1,2,3,4,5], lambda x:x&lt;2)

print( "\nar=[1,2,3,4,5], func = lambda x:x%5==0 " )
Test_remove_if([1,2,3,4,5], lambda x:x%5==0 )

print( "\nar=[1,2,3,4,5,6], func = lambda x:x%2==0 " )
Test_remove_if([1,2,3,4,5,6], lambda x:x%2==0 )

