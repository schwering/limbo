#!/usr/bin/env python

import sys

for arg in sys.argv[1:]:
    n = 0
    print "// game %s" % arg
    for c in arg:
        if ord('1') <= ord(c) and ord(c) <= ord('9'):
            print "KB: val(n%d,n%d)=n%d" % (
                    (n % 9) + 1,
                    (n / 9) + 1,
                    ord(c) - ord('0'))
        n = n + 1

