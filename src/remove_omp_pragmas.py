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

# assume first line is always this include
sys.stdout.write('#include "hclib.h"\n')

lines = []
line = sys.stdin.readline()
prev_stripped = None
while len(line) > 0:
    lines.append(line)
    stripped = line.strip()
    tokens = stripped.split()

    if len(tokens) >= 3 and tokens[0] == '#pragma' and tokens[1] == 'omp' and \
                      tokens[2] == 'critical':
        assert prev_stripped.startswith('# ')
        line_no = int(prev_stripped.split()[1])

        if line_no in handled:
            sys.stdout.write('pthread_mutex_t critical_' + str(line_no) +
                    '_lock = PTHREAD_MUTEX_INITIALIZER;\n')

    prev_stripped = line.strip()
    line = sys.stdin.readline()


prev_stripped = None
line_index = 1 # skip hclib include as we already output it above
assert lines[0].strip() == '#include "hclib.h"'

while line_index < len(lines):
    line = lines[line_index]
    line_index += 1

    stripped = line.strip()
    tokens = stripped.split()

    # Handles both OMP and omp_to_hclib pragmas
    if len(tokens) >= 2 and tokens[0] == '#pragma':
        assert prev_stripped.startswith('# ')
        line_no = int(prev_stripped.split()[1])

        if line_no in handled:
            while stripped.endswith('\\'):
                line = lines[line_index]
                line_index += 1
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

handled_fp.close()
