#!/usr/bin/python
import os
import sys

line = sys.stdin.readline()
while len(line) > 0:
    stripped = line.strip()
    tokens = stripped.split()

    if len(tokens) > 0 and tokens[0] == '#pragma':
        # This could break if or for statements that don't have compound bodies,
        # should check for this scenario in the transform
        acc = ''
        while stripped.endswith('\\'):
            acc += stripped[0:len(stripped) - 1]
            line = sys.stdin.readline()
            stripped = line.strip()
        acc += stripped

        tokens = acc.split()
        sys.stdout.write('hclib_pragma_marker("' + tokens[1] + '", "' + (' '.join(tokens[2:])) + '");\n')
    else:
        sys.stdout.write(line)

    line = sys.stdin.readline()
