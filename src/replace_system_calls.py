#!/usr/bin/python
import os
import sys

replacements = {}
for arg in sys.argv[1:]:
    tokens = arg.split(':')
    assert len(tokens) == 2
    assert not tokens[0] in replacements
    replacements[tokens[0]] = tokens[1]

for line in sys.stdin:
    for r in replacements:
        start_search = 0
        found = line.find(r, start_search)
        while found != -1:
            i = found + len(r)
            while i < len(line) and line[i] == ' ':
                i += 1

            if i < len(line) and line[i] == '(': # is function call
                line = line[0:found] + replacements[r] + line[found + len(r):]
                start_search = found + len(replacements[r])
            else:
                start_search = i

            found = line.find(r, start_search)

    sys.stdout.write(line)
