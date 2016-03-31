#!/usr/bin/python

import os
import sys

if len(sys.argv) != 2:
    print('usage: python remove_omp_pragmas.py handled-pragmas-info-file')
    sys.exit(1)

handled_file = sys.argv[1]
handled_fp = open(handled_file, 'r')

handled = {}
for line in handled_fp:
    tokens = line.split()
    assert len(tokens) == 2
    assert int(tokens[0]) not in handled

    handled[int(tokens[0])] = tokens[1]

prev_stripped = None
line = sys.stdin.readline()
while len(line) > 0:
    stripped = line.strip()
    tokens = stripped.split()

    # Handles both OMP and omp_to_hclib pragmas
    if len(tokens) >= 2 and tokens[0] == '#pragma':
        assert prev_stripped.startswith('# ')
        line_no = int(prev_stripped.split()[1])

        if line_no in handled:
            while stripped.endswith('\\'):
                line = sys.stdin.readline()
                stripped = line.strip()

            if handled[line_no] == 'taskwait':
                sys.stdout.write('hclib_end_finish(); hclib_start_finish();\n')
        else:
            sys.stdout.write(line)
    elif len(tokens) == 2 and tokens[0] == '#':
        # remove inserted line number pragmas
        pass
    else:
        sys.stdout.write(line)

    prev_stripped = line.strip()
    line = sys.stdin.readline()

handled_fp.close()
