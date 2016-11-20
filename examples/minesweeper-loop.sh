max=$1
for seed in $(seq 1 $max); do examples/Minesweeper 8 8 10 $seed; done

