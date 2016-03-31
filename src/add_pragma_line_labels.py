#!/usr/bin/python
import os
import sys

line_no = 1
for line in sys.stdin:
    if line.strip().startswith('#pragma'):
        sys.stdout.write('# ' + str(line_no) + '\n')
    sys.stdout.write(line)
    line_no += 1
