OUT=$1
if [ "$1" == "" ]
then
    OUT=demo.zip
fi

if [[ $OUT == *.zip ]]
then
    CMD="zip -r $OUT"
elif [[ $OUT == *.tar ]]
then
    CMD="tar cvf $OUT"
else
    CMD="tar zcf $OUT"
fi

$CMD \
    index.html jquery*.js \
    minesweeper/minesweeper-js.{html,js,js.mem} minesweeper/*.svg \
    sudoku/sudoku-js.{html,js,js.mem} sudoku/sudokus.txt \
    tui/tui-js.{html,js,js.mem} tui/jquery.terminal*.{js,css} tui/unix_formatting.js tui/*.lela

