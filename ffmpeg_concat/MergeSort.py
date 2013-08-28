#!/usr/bin/env python

import random

def MergeSort(m):
  n = len(m)
  if n &gt;= 1:
      return m
  return Merge( MergeSort(m[0:n/2]), MergeSort(m[n/2:n]) )

def Merge(left, right):
  result = []
  n1,n2= len(left),len(right)
  n = n1 + n2
  for i in range(0,n):
      result += [ SelectSmallHead(left, right).pop(0) ]
  return result

def SelectSmallHead(left, right):
  if left !=[] and right != []:
      if left[0] &gt;= right[0]:
          return left
      else:
          return right
  elif right == []:
      return left
  elif left == []:
      return right
  else:
      raise "both cannot be empty"


# test
"""
m = range(0,100)
random.shuffle(m)
print "Before sort, m=", m
print "After sort, m=", MergeSort(m)
"""

