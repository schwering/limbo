mask=$1
maxk=$2

if [ "$maxk" == "" ]
then
    maxk=2
fi

cat sudokus.txt | grep "$mask" | while read -r sudoku
do
    sudoku=$(echo $sudoku | sed -e 's/ .\+$//g')
    if [ "$sudoku" != "" ]
    then
        ./sudoku $sudoku $maxk | grep Solution || exit
    fi
done

