#!/usr/bin/python
import os
import sys

if len(sys.argv) != 3:
    print('usage: python insert_critical_locks.py start end')
    sys.exit(1)

start = int(sys.argv[1])
stop = int(sys.argv[2])

expected_header = ['#include "hclib.h"',
                   '#ifdef __cplusplus',
                   '#include "hclib_cpp.h"',
                   '#include "hclib_system.h"',
                   '#include "hclib_openshmem.h"',
                   '#ifdef __CUDACC__',
                   '#include "hclib_cuda.h"',
                   '#endif',
                   '#endif',
                   'extern void hclib_pragma_marker(const char *pragma_name, const char *pragma_arguments);']


for expected in expected_header:
    line = sys.stdin.readline()
    assert line.strip() == expected
    sys.stdout.write(line)

index = start
while index < stop:
    sys.stdout.write('pthread_mutex_t critical_' + str(index) +
            '_lock = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;\n')
    index += 1

line = sys.stdin.readline()
while len(line) > 0:
    sys.stdout.write(line)
    line = sys.stdin.readline()
