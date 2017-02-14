W=$1
H=$2
M=$3
K=$4
max=$5
F=mw-${W}x${H}-${M}-${K}-${max}
for seed in $(seq 101 $max); do ./mw $W $H $M $seed $K; done | grep '\(Last move\|Exploring\|Flagging\|You\)' >>$F
echo $F

