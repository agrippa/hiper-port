#!/bin/bash

set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

FILE_PREFIX=____omp_to_hclib

OPENMP_FINDER=$SCRIPT_DIR/openmp_finder.py
OMP_TO_HCLIB=$SCRIPT_DIR/omp_to_hclib/omp_to_hclib
REMOVE_OMP_PRAGMAS=$SCRIPT_DIR/remove_omp_pragmas.py
OMP_TO_HCLIB_PRAGMA_FINDER=$SCRIPT_DIR/omp_to_hclib_pragma_finder.py
ADD_PRAGMA_LINE_LABELS=$SCRIPT_DIR/add_pragma_line_labels.py

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

[[ $VERBOSE == 1 ]] && echo 'DEBUG >>> Prepending header'
echo '#include "hclib.h"' > $WITH_HEADER
cat $INPUT_PATH >> $WITH_HEADER

COUNT=1
CHANGED=1
PREV=$WITH_HEADER
CHECK_FOR_PTHREAD=true
until [[ $CHANGED -eq 0 ]]; do
    OMP_INFO=$DIRNAME/$FILE_PREFIX.$NAME.omp.$COUNT.info
    OMP_TO_HCLIB_INFO=$DIRNAME/$FILE_PREFIX.$NAME.omp_to_hclib.$COUNT.info
    WITH_LINE_LABELS=$DIRNAME/$FILE_PREFIX.$NAME.line_labels.$COUNT.$EXTENSION
    TMP_OUTPUT=$DIRNAME/$FILE_PREFIX.$NAME.hclib.$COUNT.$EXTENSION
    WITHOUT_PRAGMAS=$DIRNAME/$FILE_PREFIX.$NAME.no_pragmas.$COUNT.$EXTENSION
    HANDLED_PRAGMAS_INFO=$DIRNAME/$FILE_PREFIX.pragmas.$COUNT.info

    # Find all uses of OpenMP pragrams in the input file and store them in $OMP_INFO
    [[ $VERBOSE == 1 ]] && echo 'DEBUG >>> Finding OMP pragmas'
    python $OPENMP_FINDER $PREV > $OMP_INFO

    # Search for pragmas specific to this framework
    [[ $VERBOSE == 1 ]] && echo 'DEBUG >>> Finding omp_to_hclib-specific pragmas'
    cat $PREV | python $OMP_TO_HCLIB_PRAGMA_FINDER > $OMP_TO_HCLIB_INFO

    [[ $VERBOSE == 1 ]] && echo 'DEBUG >>> Adding line labels to pragmas'
    cat $PREV | python $ADD_PRAGMA_LINE_LABELS > $WITH_LINE_LABELS

    # Translate OMP pragmas detected by OPENMP_FINDER into HClib constructs.
    [[ $VERBOSE == 1 ]] && echo 'DEBUG >>> Converting OMP parallelism to HClib'
    $OMP_TO_HCLIB -o $TMP_OUTPUT -m $OMP_INFO -s $OMP_TO_HCLIB_INFO $WITH_LINE_LABELS \
        -d $HANDLED_PRAGMAS_INFO -n $CHECK_FOR_PTHREAD -- $INCLUDE $USER_INCLUDES $DEFINES \
        -I$HCLIB_ROOT/include

    # Remove any OMP pragmas that this iteration handled
    [[ $VERBOSE == 1 ]] && echo 'DEBUG >>> Removing OMP pragmas'
    cat $TMP_OUTPUT | python $REMOVE_OMP_PRAGMAS $HANDLED_PRAGMAS_INFO > $WITHOUT_PRAGMAS

    # Check to see if there were any changes to the code during this iteration. If not, exit.
    set +e
    diff $WITH_LINE_LABELS $TMP_OUTPUT > /dev/null
    CHANGED=$?
    set -e

    COUNT=$(($COUNT + 1))
    PREV=$WITHOUT_PRAGMAS
    CHECK_FOR_PTHREAD=false
done

[[ $VERBOSE == 1 ]] && echo 'DEBUG >>> Producing final output file'
cp $WITHOUT_PRAGMAS $OUTPUT_PATH

if [[ $KEEP == 0 ]]; then
    [[ $VERBOSE == 1 ]] && echo 'DEBUG >>> Cleaning up'
    rm -f $DIRNAME/$FILE_PREFIX.*
fi
