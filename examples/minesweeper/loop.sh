max=$1
maxk=$2
for seed in $(seq 1 $max); do ./mw 16 16 40 $seed $maxk; done

