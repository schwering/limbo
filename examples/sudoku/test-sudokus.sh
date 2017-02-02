for sudoku in $(cat sudoku-js.html | grep option | sed -e 's/^.\+="//g' | sed -e 's/".\+$//g' | grep -v '[a-z]')
do
    ./sudoku $sudoku 2 | grep Solution || exit
done

