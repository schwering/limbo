sed -ue 's/.\+(in //g' | \
sed -ue 's/s)//g' | \
sed -ue 's/s, sat-cdcl)//g' | \
python3 -c "
import sys
sum = 0
cnt = 0
min = -1
mini = -1
max = -1
maxi = -1
for line in sys.stdin:
    f = float(line.strip())
    sum = sum + f
    cnt = cnt + 1
    if min < 0 or min > f:
        min = f
        mini = cnt
    if max < 0 or max < f:
        max = f
        maxi = cnt
    print('%.5f / %4d = %.5f | min = %.5f (%d) | max = %.5f (%d)' % (sum, cnt, sum/cnt, min, mini, max, maxi), flush=True, end='\r')
print('')
"

