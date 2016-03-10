#!/usr/bin/python
import os
import sys

# This script inserts preprocessor line numbers on every line of the input file.
# This ensures that line numbers for statements are retained except for
# statements that are collapsed into a single line.

if len(sys.argv) != 2:
    print('usage: python insert_line_numbers.py <main-file>')
    sys.exit(1)

# Only add instrumentation for the main input file
main_file = sys.argv[1]
curr_line = 1
enabled = False

for line in sys.stdin:
    if line.find('# ') == 0:
        tokens = line.split()
        curr_line = int(tokens[1]) - 1

        if len(tokens) > 2 and tokens[2] == '"' + main_file + '"':
            enabled = True
        else:
            enabled = False
    else:
        if enabled:
            sys.stdout.write('# ' + str(curr_line) + '\n')

    sys.stdout.write(line)

    curr_line = curr_line + 1
