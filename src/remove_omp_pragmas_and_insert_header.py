#!/usr/bin/python

import os
import sys

sys.stdout.write('#include "hclib.h"\n')

line = sys.stdin.readline()
while len(line) > 0:
    stripped = line.strip()
    tokens = stripped.split()

    if len(tokens) >= 2 and tokens[0] == '#pragma' and tokens[1] == 'omp':
        while stripped.endswith('\\'):
            line = sys.stdin.readline()
            stripped = line.strip()
    else:
        sys.stdout.write(line)

    line = sys.stdin.readline()
