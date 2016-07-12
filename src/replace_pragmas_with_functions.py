#!/usr/bin/python
import os
import sys

only_omp_to_hclib = False
if len(sys.argv) == 2:
    only_omp_to_hclib = (sys.argv[1] == 'true')

line_no = 1
line = sys.stdin.readline()
while len(line) > 0:
    stripped = line.strip()
    tokens = stripped.split()
    handled = False

    if len(tokens) > 0 and tokens[0] == '#pragma':
        # This could break if or for statements that don't have compound bodies,
        # should check for this scenario in the transform
        if not only_omp_to_hclib or tokens[1] == 'omp_to_hclib':
            handled = True

            acc = ''
            while stripped.endswith('\\'):
                acc += stripped[0:len(stripped) - 1] + ' '
                line = sys.stdin.readline()
                line_no += 1
                stripped = line.strip()
            acc += stripped

            tokens = acc.split()

            pragma_lbl = 'pragma' + str(line_no) + '_' + tokens[1]
            if len(tokens) > 2:
                pragma_lbl += '_' + tokens[2]

            sys.stdout.write('hclib_pragma_marker("' + tokens[1] + '", "' +
                             (' '.join(tokens[2:])) + '", "' + pragma_lbl +
                             '");\n')

    if not handled:
        sys.stdout.write(line)

    line = sys.stdin.readline()
    line_no += 1
