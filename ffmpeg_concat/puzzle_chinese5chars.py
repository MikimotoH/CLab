#!/usr/bin/env python

revStr = lambda s:s[len(s)-1:0:-1]+s[0]
revInt = lambda n:int(revStr(str(n)))
isPalin = lambda a,b:revInt(a)==(b)
is4div = lambda n:n%4==0
biglist = range(1,99999)
quadMul = filter(is4div, biglist)
isQuoPalin = lambda n:isPalin(n/4,n)
blist = filter(isQuoPalin, quadMul)

"""
>>> print blist
[8712, 87912]
"""


