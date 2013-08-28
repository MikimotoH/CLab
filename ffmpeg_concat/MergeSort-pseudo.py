var integer middle = length(m) / 2
for each x in m up to middle
    add x to left
for each x in m after middle
    add x to right
left = merge_sort(left)
right = merge_sort(right)
result = merge(left, right)
return result


    left= m[0:n/2]
    right = m[n/2:n]

