#!/usr/bin/python
import os
import sys

#
# This script searches a C file for all OMP pragmas and outputs their starting
# line number, ending line number, and the inlined OMP pragma (removing
# backslashes if it is across multiple lines).
#

if __name__ == '__main__':
    if len(sys.argv) != 2:
        print('usage: python openmp_finder.py input')
        sys.exit(1)

    input_filename = sys.argv[1]

    input_fp = open(input_filename, 'r')
    
    # This line will throw an exception similar to:
    #     UnicodeDecodeError: 'utf8' codec can't decode byte 0xd6 in position \
    #         84: invalid continuation byte
    # if it finds a non-ASCII character in an input file.
    curr_line_no = 1
    curr_file = None
    line = input_fp.readline()
    while len(line) > 0:
        # For efficiency, quickly filter out all but preprocessor lines
        line = line.strip()

        if len(line) > 0 and line[0] == '#':
            tokens = line.split()
            if tokens[0] == '#':
                curr_line_no = int(tokens[1])
                if len(tokens) > 2:
                    curr_file = tokens[2][1:len(tokens[2]) - 1]
            elif tokens[0] == '#pragma' and tokens[1] == 'omp':
                acc = line
                starting_line_no = curr_line_no
                while line[len(line) - 1] == '\\':

                    line = input_fp.readline().strip()
                    line_tokens = line.split()
                    while len(line_tokens) > 0 and line_tokens[0] == '#':
                        curr_line_no = int(line_tokens[1])
                        if len(line_tokens) > 2:
                            curr_file = line_tokens[2][1:len(line_tokens[2]) - 1]
                        line = input_fp.readline().strip()
                        line_tokens = line.split()

                    acc = acc[:len(acc) - 1].strip() # trim the last \
                    acc += ' ' + line
                sys.stdout.write(str(starting_line_no) + ' ' + str(curr_line_no) + ' ' + acc + '\n')
            else:
                curr_line_no += 1
        else:
            curr_line_no += 1

        line = input_fp.readline()

    input_fp.close()
