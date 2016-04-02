#!/bin/bash

set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

FILE_PREFIX=____omp_to_hclib

OMP_TO_HCLIB=$SCRIPT_DIR/omp_to_hclib/omp_to_hclib
REPLACE_PRAGMAS_WITH_FUNCTIONS=$SCRIPT_DIR/replace_pragmas_with_functions.py
INSERT_CRITICAL_LOCKS=$SCRIPT_DIR/insert_critical_locks.py

GXX="$GXX"
if [[ -z "$GXX" ]]; then
    GXX=g++
fi

INPUT_PATH=
OUTPUT_PATH=
KEEP=0
VERBOSE=0
USER_INCLUDES=

while getopts "i:o:kvhI:" opt; do
    case $opt in 
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
DEFINES=-D_FORTIFY_SOURCE=0

WITH_HEADER=$DIRNAME/$FILE_PREFIX.$NAME.header.$EXTENSION
WITH_PRAGMA_MARKERS=$DIRNAME/$FILE_PREFIX.$NAME.pragma_markers.$EXTENSION

[[ $VERBOSE == 1 ]] && echo 'DEBUG >>> Prepending header'
echo '#include "hclib.h"' > $WITH_HEADER
cat $INPUT_PATH >> $WITH_HEADER

[[ $VERBOSE == 1 ]] && echo 'DEBUG >>> Inserting pragma markers'
cat $WITH_HEADER | python $REPLACE_PRAGMAS_WITH_FUNCTIONS > $WITH_PRAGMA_MARKERS

COUNT=1
CHANGED=1
PREV=$WITH_PRAGMA_MARKERS
CHECK_FOR_PTHREAD=true
CRITICAL_SECTION_ID=0
until [[ $CHANGED -eq 0 ]]; do
    OMP_INFO=$DIRNAME/$FILE_PREFIX.$NAME.omp.$COUNT.info
    OMP_TO_HCLIB_INFO=$DIRNAME/$FILE_PREFIX.$NAME.omp_to_hclib.$COUNT.info
    WITH_LINE_LABELS=$DIRNAME/$FILE_PREFIX.$NAME.line_labels.$COUNT.$EXTENSION
    TMP_OUTPUT=$DIRNAME/$FILE_PREFIX.$NAME.hclib.$COUNT.$EXTENSION
    WITHOUT_PRAGMAS=$DIRNAME/$FILE_PREFIX.$NAME.no_pragmas.$COUNT.$EXTENSION
    HANDLED_PRAGMAS_INFO=$DIRNAME/$FILE_PREFIX.handled_pragmas.$COUNT.info
    CRITICAL_SECTION_ID_FILE=$DIRNAME/$FILE_PREFIX.critical_id.$COUNT.info
    WITH_LOCKS=$DIRNAME/$FILE_PREFIX.$NAME.with_locks.$COUNT.$EXTENSION

    # Translate OMP pragmas detected into HClib constructs.
    [[ $VERBOSE == 1 ]] && echo 'DEBUG >>> Converting OMP parallelism to HClib'
    $OMP_TO_HCLIB -o $TMP_OUTPUT -c $CRITICAL_SECTION_ID \
        -r $CRITICAL_SECTION_ID_FILE -n $CHECK_FOR_PTHREAD $PREV -- $INCLUDE \
        $USER_INCLUDES $DEFINES -I$HCLIB_ROOT/include

    PREV_CRITICAL_SECTION_ID=$CRITICAL_SECTION_ID
    CRITICAL_SECTION_ID=$(cat $CRITICAL_SECTION_ID_FILE)

    # Insert critical/atomic section locks
    [[ $VERBOSE == 1 ]] && echo 'DEBUG >>> Inserting critical section locks'
    cat $TMP_OUTPUT | python $INSERT_CRITICAL_LOCKS $PREV_CRITICAL_SECTION_ID $CRITICAL_SECTION_ID > $WITH_LOCKS

    # Check to see if there were any changes to the code during this iteration. If not, exit.
    set +e
    diff $WITH_LOCKS $PREV > /dev/null
    CHANGED=$?
    set -e

    COUNT=$(($COUNT + 1))
    PREV=$WITH_LOCKS
    CHECK_FOR_PTHREAD=false
done

[[ $VERBOSE == 1 ]] && echo 'DEBUG >>> Producing final output file'
cp $PREV $OUTPUT_PATH

if [[ $KEEP == 0 ]]; then
    [[ $VERBOSE == 1 ]] && echo 'DEBUG >>> Cleaning up'
    rm -f $DIRNAME/$FILE_PREFIX.*
fi
