W=$1
H=$2
M=$3
K=$4
max=$5
F=test-${W}x${H}-${M}-${K}-${max}.log

inxi -Fx -c 0 >$F

for seed in $(seq 1 $max); do ./minesweeper $W $H $M $seed $K | grep You >>$F; done
echo $F

