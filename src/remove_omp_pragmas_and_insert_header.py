#!/usr/bin/python

import os
import sys

sys.stdout.write('#include "hclib.h"\n')

for line in sys.stdin:
    stripped = line.strip()
    tokens = stripped.split()

    if len(tokens) >= 2 and tokens[0] == '#pragma' and tokens[1] == 'omp':
        pass
    else:
        sys.stdout.write(line)
