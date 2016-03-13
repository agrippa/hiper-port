#!/usr/bin/python

import os
import sys

start_line = -1
end_line = -1

line_no = 1
for line in sys.stdin:
    stripped = line.strip()
    tokens = stripped.split()

    if len(tokens) == 3 and tokens[0] == '#pragma' and tokens[1] == 'omp_to_hclib':
        if tokens[2] == 'body_start':
            assert start_line == -1
            start_line = line_no
        elif tokens[2] == 'body_end':
            assert end_line == -1
            end_line = line_no
    line_no += 1

if start_line != -1 or end_line != -1:
    assert start_line != -1 and end_line != -1
    print('BODY ' + str(start_line) + ' ' + str(end_line))
