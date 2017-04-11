F=$1

for sudoku in $(cat $F)
do
    echo -n "Preset cells: "
    echo -n $sudoku | sed 's/\.//g' | wc -c
    ./sudoku $sudoku 2 | grep Solution
done

