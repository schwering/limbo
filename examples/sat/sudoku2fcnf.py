#!/usr/bin/python3

import sys

def to_index(x,y = None):
    if isinstance(x, tuple):
        (x,y) = x
    return x + (y - 1) * 9;

def to_point(i): 
    x = ((i-1) % 9) + 1
    y = ((i-1) // 9) + 1
    return (x,y)

sudoku = sys.argv[1]

fcs = set()

# Clues
for i in range(0, len(sudoku)):
    if '1' <= sudoku[i] <= '9':
        v = ord(sudoku[i]) - ord('0')
        fc = frozenset([(i+1, v)])
        fcs.add(fc)

# Sudoku rule: every cell has value 1..9
for x in range(1,10):
    for y in range(1,10):
        fc = frozenset([(to_index(x,y),v) for v in range(1, 10)])
        fcs.add(fc)

# Sudoku rule: in every column, no two cells have the same value
for x in range(1, 10):
    for xx in range(1, x):
        for y in range(1, 10):
            for v in range(1, 10):
                fc = frozenset([(-to_index(x,y),v), (-to_index(xx,y),v)])
                fcs.add(fc)

# Sudoku rule: in every row, no two cells have the same value
for x in range(1, 10):
    for y in range(1, 10):
        for yy in range(1, y):
            for v in range(1, 10):
                fc = frozenset([(-to_index(x,y),v), (-to_index(x,yy),v)])
                fcs.add(fc)

# Sudoku rule: in every box, no two cells have the same value
for i in range(1,4):
    for j in range(1,4):
        for x in range(3*i-2, 3*i+1):
            for xx in range(3*i-2, 3*i+1):
                for y in range(3*j-2, 3*j+1):
                    for yy in range(3*j-2, 3*j+1):
                        if (x,y) != (xx,yy):
                            for v in range(1, 10):
                                fc = frozenset([(-to_index(x,y),v), (-to_index(xx,yy),v)])
                                fcs.add(fc)


print("p fcnf %d %d %d" % (81, 9, len(fcs)))
for fc in sorted(fcs, key=len):
    for (lhs,rhs) in fc:
        print("%3d = %1d   " % (lhs,rhs), end='')
    print("%d" % (0,))

