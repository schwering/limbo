max=$1
for seed in $(seq 1 $max); do examples/Minesweeper 16 16 40 $seed; done

