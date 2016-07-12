#!/bin/bash

set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

FILE_PREFIX=____time_body

TIME_BODY=$SCRIPT_DIR/time_body/time_body
REPLACE_PRAGMAS_WITH_FUNCTIONS=$SCRIPT_DIR/replace_pragmas_with_functions.py

GXX="$GXX"
if [[ -z "$GXX" ]]; then
    GXX=g++
fi

INPUT_PATH=
OUTPUT_PATH=
KEEP=0
VERBOSE=0
USER_INCLUDES=
USER_DEFINES=
TARGET_LANG=HCLIB
TIMING_SCOPE_FLAGS=

while getopts "i:o:kvhI:D:l:fp" opt; do
    case $opt in 
        f)
            TIMING_SCOPE_FLAGS="${TIMING_SCOPE_FLAGS} -f enable"
            ;;
        p)
            TIMING_SCOPE_FLAGS="${TIMING_SCOPE_FLAGS} -l enable"
            ;;
        l)
            TARGET_LANG=$OPTARG
            ;;
        i)
            INPUT_PATH=$OPTARG
            ;;
        o)
            OUTPUT_PATH=$OPTARG
            ;;
        k)
            KEEP=1
            ;;
        v)
            VERBOSE=1
            ;;
        I)
            USER_INCLUDES="$USER_INCLUDES -I$OPTARG"
            ;;
        D)
            USER_DEFINES="$USER_DEFINES -D$OPTARG"
            ;;
        h)
            echo 'usage: omp_to_hclib.sh <-i input-file> <-o output-file> [-k] [-v] [-h] [-I include-path]'
            exit 1
            ;;
        \?)
            echo "unrecognized option -$OPTARG" >&2
            exit 1
            ;;
        :)
            echo "option -$OPTARG requires an argument" >&2
            exit 1
            ;;
    esac
done

if [[ -z "${TIMING_SCOPE_FLAGS}" ]]; then
    TIMING_SCOPE_FLAGS="-f enable"
fi

if [[ -z "$INPUT_PATH" ]]; then
    echo 'Missing input path, must be provided with -i'
    exit 1
fi

if [[ -z "$OUTPUT_PATH" ]]; then
    echo 'Missing output path, must be provided with -o'
    exit 1
fi

if [[ $VERBOSE == 1 ]]; then
    echo INPUT_PATH = $INPUT_PATH
    echo OUTPUT_PATH = $OUTPUT_PATH
    echo KEEP = $KEEP
    echo USER_DEFINES = $USER_DEFINES
    echo VERBOSE = $VERBOSE
fi

DIRNAME=$(dirname $INPUT_PATH)
FILENAME=$(basename $INPUT_PATH)
EXTENSION="${FILENAME##*.}"
NAME="${FILENAME%.*}"

IS_CPP=0
if [[ $EXTENSION == 'cpp' ]]; then
    IS_CPP=1
fi

INCLUDE=""

for DIR in $(cpp -v < /dev/null 2>&1 | awk 'BEGIN { doPrint = 0; } /#include <...> search starts here:/ { doPrint = 1; } /End of search list/ { doPrint = 0; } { if (doPrint) print $0; }' | tail -n +2); do
    INCLUDE="$INCLUDE -I$DIR"
done

if [[ $IS_CPP -eq 1 && -d /usr/include/c++ ]]; then
    for DIR in $(ls /usr/include/c++); do
        INCLUDE="$INCLUDE -I/usr/include/c++/$DIR"
    done
fi

# We add -D_FORTIFY_SOURCE here for Mac OS where not setting it to zero causes
# many utility functions (e.g. memcpy, strcpy) to be replaced with intrinsics
# from /usr/include/secure/_string.h. While this is fine at compile time, it
# breaks tests that assert the generated code is identical to the reference
# output.
DEFINES="-D_FORTIFY_SOURCE=0 $USER_DEFINES"

WITH_PRAGMA_MARKERS=$DIRNAME/$FILE_PREFIX.$NAME.pragma_markers.$EXTENSION

[[ $VERBOSE == 1 ]] && echo 'DEBUG >>> Inserting pragma markers'
echo 'extern void hclib_pragma_marker(const char *pragma_name, const char *pragma_arguments);' >> $WITH_PRAGMA_MARKERS
cat $INPUT_PATH | python $REPLACE_PRAGMAS_WITH_FUNCTIONS >> $WITH_PRAGMA_MARKERS

COUNT=1
CHANGED=1
PREV=$WITH_PRAGMA_MARKERS
until [[ $CHANGED -eq 0 ]]; do
    OMP_INFO=$DIRNAME/$FILE_PREFIX.$NAME.omp.$COUNT.info
    TMP_OUTPUT=$DIRNAME/$FILE_PREFIX.$NAME.hclib.$COUNT.$EXTENSION
    WITHOUT_PRAGMAS=$DIRNAME/$FILE_PREFIX.$NAME.no_pragmas.$COUNT.$EXTENSION
    HANDLED_PRAGMAS_INFO=$DIRNAME/$FILE_PREFIX.handled_pragmas.$COUNT.info

    # Translate OMP pragmas detected into HClib constructs.
    [[ $VERBOSE == 1 ]] && echo 'DEBUG >>> Converting OMP parallelism to HClib'
    $TIME_BODY -o $TMP_OUTPUT ${TIMING_SCOPE_FLAGS} $PREV -- $INCLUDE \
        $USER_INCLUDES $DEFINES

    # Check to see if there were any changes to the code during this iteration. If not, exit.
    set +e
    diff $TMP_OUTPUT $PREV > /dev/null
    CHANGED=$?
    set -e
    [[ $VERBOSE == 1 ]] && echo "DEBUG >>> Any change from $PREV to $TMP_OUTPUT? $CHANGED"

    COUNT=$(($COUNT + 1))
    PREV=$TMP_OUTPUT
done

N_PRAGMA_MARKERS=$(cat $PREV | grep "^hclib_pragma_marker" | wc -l)
if [[ $N_PRAGMA_MARKERS -ne 0 ]]; then
    echo Unexpected number of leftover pragma markers \($N_PRAGMA_MARKERS\) in \
        $PREV, should only be the top-level declaration
    exit 1
fi

cat $PREV | grep -v "^extern void hclib_pragma_marker" > ${OUTPUT_PATH}.body

[[ $VERBOSE == 1 ]] && echo "DEBUG >>> Producing final output file at $OUTPUT_PATH from $PREV"
echo '#include <sys/time.h>
#include <time.h>
static unsigned long long current_time_ns() {
#ifdef __MACH__
    clock_serv_t cclock;
    mach_timespec_t mts;
    host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
    clock_get_time(cclock, &mts);
    mach_port_deallocate(mach_task_self(), cclock);
    unsigned long long s = 1000000000ULL * (unsigned long long)mts.tv_sec;
    return (unsigned long long)mts.tv_nsec + s;
#else
    struct timespec t ={0,0};
    clock_gettime(CLOCK_MONOTONIC, &t);
    unsigned long long s = 1000000000ULL * (unsigned long long)t.tv_sec;
    return (((unsigned long long)t.tv_nsec)) + s;
#endif
}' > ${OUTPUT_PATH}.header
cat ${OUTPUT_PATH}.header ${OUTPUT_PATH}.body > $OUTPUT_PATH

if [[ $KEEP == 0 ]]; then
    [[ $VERBOSE == 1 ]] && echo 'DEBUG >>> Cleaning up'
    rm -f $DIRNAME/$FILE_PREFIX.*
fi
