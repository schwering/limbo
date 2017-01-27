#!/usr/bin/env python

import sys

for arg in sys.argv[1:]:
    print "// %s" % arg
    if len(arg) != 9*9:
        print "// CAUTION: game has only length %d" % len(arg)

    n = 0
    for c in arg:
        if n % 9 == 0:
            print "// ",
        if ord('1') <= ord(c) and ord(c) <= ord('9'):
            print (ord(c) - ord('0')),
        else:
            print " ",
        n = n + 1
        if n % 9 == 0:
            print

    print "if K<0> game=N {"
    n = 0
    for c in arg:
        if ord('1') <= ord(c) and ord(c) <= ord('9'):
            print "    KB: val(n%d,n%d)=n%d" % (
                    (n % 9) + 1,
                    (n / 9) + 1,
                    ord(c) - ord('0'))
        n = n + 1
    print "}"

