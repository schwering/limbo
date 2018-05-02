sed -ue 's/.\+(in //g' | \
sed -ue 's/s)//g' | \
python3 -c "
import sys
sum = 0
cnt = 0
for line in sys.stdin:
    f = float(line.strip())
    sum = sum + f
    cnt = cnt + 1
    print('%.5f / %4d = %.5f' % (sum, cnt, sum/cnt), flush=True, end='\r')
print('')
"

