#!/usr/bin/python

import os
import sys


line = sys.stdin.readline()
while len(line) > 0:
    # We make a best effort to precisely retain trailing whitespace here mostly
    # to make the test diffs easier to see.
    trailing_whitespace = line[len(line.rstrip()):]
    acc = line.rstrip()

    # len(acc) > 0 conditional just in case this is an empty line, and stripping
    # removed the new line character
    while len(acc) > 0 and acc[len(acc) - 1] == '\\':
        next_line = sys.stdin.readline()
        next_line_stripped = next_line.rstrip()
        trailing_whitespace = next_line[len(next_line_stripped):]

        acc = acc[:len(acc) - 1] + ' ' + next_line_stripped

    tokens = acc.split()
    if len(tokens) >= 3 and tokens[0] == '#pragma' and tokens[1] == 'omp' and tokens[2] == 'task':
        for i in range(len(tokens)):
            if tokens[i] == 'untied':
                tokens[i] = ''

        pragma_without_untied = ' '.join(tokens)
        sys.stdout.write('#ifdef HCLIB_TASK_UNTIED\n')
        sys.stdout.write(pragma_without_untied + ' untied\n')
        sys.stdout.write('#else\n')
        sys.stdout.write(pragma_without_untied + '\n')
        sys.stdout.write('#endif\n')
    else:
        sys.stdout.write(acc + trailing_whitespace)

    line = sys.stdin.readline()
