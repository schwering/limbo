for F in "$@"
do
    echo "File: $F"

    echo -n "Solved: "
    python -c "print '%d / %d = %f' % ( $(grep -v '^#' $F | grep "You" | grep win | wc -l), $(grep -v '^#' $F | grep "You" | wc -l), $(grep -v '^#' $F | grep "You" | grep win | wc -l).0 / $(grep -v '^#' $F | grep "You" | wc -l) )"

    echo -n "Average time (seconds): "
    python -c "print $((grep -v '^#' $F | grep "You" | sed -r "s/\x1B\[([0-9]{1,2}(;[0-9]{1,2})?)?[m|K]//g" | sed -e 's/^.\+runtime://g' | sed -e 's/seconds.\+$//g' && echo 0.0) | paste -sd+ | bc) / $(grep -v '^#' $F | grep "You" | wc -l)"

    echo -n "Average time per move (seconds): "
    python -c "print '%.2E' % ($((grep -v '^#' $F | grep "Last move" | sed -e "s/^[^0-9]\+//g" | sed -e 's/,.\+//g' && echo 0.0) | paste -sd+ | bc) / $(grep -v '^#' $F | grep "Last move" | wc -l))"

    echo -n "Found at level  0 (avg per game): "
    python -c "print $(grep -v '^#' $F | grep "split level" | grep "level 0" | wc -l).0 / $(grep -v '^#' $F | grep "You" | wc -l)"

    echo -n "Found at level  1 (avg per game): "
    python -c "print $(grep -v '^#' $F | grep "split level" | grep "level 1" | wc -l).0 / $(grep -v '^#' $F | grep "You" | wc -l)"

    echo -n "Found at level  2 (avg per game): "
    python -c "print $(grep -v '^#' $F | grep "split level" | grep "level 2" | wc -l).0 / $(grep -v '^#' $F | grep "You" | wc -l)"

    echo -n "Found at level  3 (avg per game): "
    python -c "print $(grep -v '^#' $F | grep "split level" | grep "level 3" | wc -l).0 / $(grep -v '^#' $F | grep "You" | wc -l)"

    echo -n "Found at level >0 (avg per game): "
    python -c "print $(grep -v '^#' $F | grep "split level" | grep -v "level 0" | wc -l).0 / $(grep -v '^#' $F | grep "You" | wc -l)"

    echo -n "Guesses           (avg per game): "
    python -c "print $(grep -v '^#' $F | grep "guess" | wc -l).0 / $(grep -v '^#' $F | grep "You" | wc -l)"

    echo -n "Found at level  0 (rel): "
    python -c "print $(grep -v '^#' $F | grep "split level" | grep "level 0" | wc -l).0 / $(grep -v '^#' $F | grep "Last move" | wc -l)"

    echo -n "Found at level  1 (rel): "
    python -c "print $(grep -v '^#' $F | grep "split level" | grep "level 1" | wc -l).0 / $(grep -v '^#' $F | grep "Last move" | wc -l)"

    echo -n "Found at level  2 (rel): "
    python -c "print $(grep -v '^#' $F | grep "split level" | grep "level 2" | wc -l).0 / $(grep -v '^#' $F | grep "Last move" | wc -l)"

    echo -n "Found at level  3 (rel): "
    python -c "print $(grep -v '^#' $F | grep "split level" | grep "level 3" | wc -l).0 / $(grep -v '^#' $F | grep "Last move" | wc -l)"

    echo -n "Found at level >0 (rel): "
    python -c "print $(grep -v '^#' $F | grep "split level" | grep -v "level 0" | wc -l).0 / $(grep -v '^#' $F | grep "Last move" | wc -l)"

    echo -n "Guesses           (rel): "
    python -c "print $(grep -v '^#' $F | grep "guess" | wc -l).0 / $(grep -v '^#' $F | grep "Last move" | wc -l)"

    echo ""
done

