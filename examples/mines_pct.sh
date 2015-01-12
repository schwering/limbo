F=$1

echo -n "Solved: "
python -c "print $(grep "You" $F | grep win | wc -l).0 / $(grep "You" $F | wc -l)"

echo -n "Average time (seconds): "
python -c "print ($(grep "You" $F | sed -r "s/\x1B\[([0-9]{1,2}(;[0-9]{1,2})?)?[m|K]//g" | sed -e 's/^.\+runtime://g' | sed -e 's/seconds.\+$/ + \\/g' && echo 0)) / $(grep "You" $F | wc -l)"

echo -n "Found at level  0 (avg per game): "
python -c "print $(grep "split level" $F | grep "level 0" | wc -l).0 / $(grep "You" $F | wc -l)"

echo -n "Found at level  1 (avg per game): "
python -c "print $(grep "split level" $F | grep "level 1" | wc -l).0 / $(grep "You" $F | wc -l)"

echo -n "Found at level  2 (avg per game): "
python -c "print $(grep "split level" $F | grep "level 2" | wc -l).0 / $(grep "You" $F | wc -l)"

echo -n "Found at level  3 (avg per game): "
python -c "print $(grep "split level" $F | grep "level 3" | wc -l).0 / $(grep "You" $F | wc -l)"

echo -n "Found at level >0 (avg per game): "
python -c "print $(grep "split level" $F | grep -v "level 0" | wc -l).0 / $(grep "You" $F | wc -l)"

echo -n "Guesses           (avg per game): "
python -c "print $(grep "guess" $F | wc -l).0 / $(grep "You" $F | wc -l)"

echo -n "Found at level  0 (rel): "
python -c "print $(grep "split level" $F | grep "level 0" | wc -l).0 / $(grep "X and Y coordinates" $F | wc -l)"

echo -n "Found at level  1 (rel): "
python -c "print $(grep "split level" $F | grep "level 1" | wc -l).0 / $(grep "X and Y coordinates" $F | wc -l)"

echo -n "Found at level  2 (rel): "
python -c "print $(grep "split level" $F | grep "level 2" | wc -l).0 / $(grep "X and Y coordinates" $F | wc -l)"

echo -n "Found at level  3 (rel): "
python -c "print $(grep "split level" $F | grep "level 3" | wc -l).0 / $(grep "X and Y coordinates" $F | wc -l)"

echo -n "Found at level >0 (rel): "
python -c "print $(grep "split level" $F | grep -v "level 0" | wc -l).0 / $(grep "X and Y coordinates" $F | wc -l)"

echo -n "Guesses           (rel): "
python -c "print $(grep "guess" $F | wc -l).0 / $(grep "X and Y coordinates" $F | wc -l)"


