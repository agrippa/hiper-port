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

while getopts "i:o:kvhI:D:" opt; do
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
        D)
            USER_DEFINES="$USER_DEFINES -D$OPTARG"
            ;;
        h)
            echo 'usage: time_body.sh <-i input-file> <-o output-file> [-k] [-v] [-h] [-I include-path]'
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

WITH_HEADER=$DIRNAME/$FILE_PREFIX.$NAME.header.$EXTENSION
WITH_PRAGMA_MARKERS=$DIRNAME/$FILE_PREFIX.$NAME.pragma_markers.$EXTENSION

[[ $VERBOSE == 1 ]] && echo 'DEBUG >>> Prepending header'
echo '#include "hclib.h"' > $WITH_HEADER
echo 'extern void hclib_pragma_marker(const char *pragma_name, const char *pragma_arguments);' >> $WITH_HEADER
# INIT_STR='{ 0'
# for I in $(seq 31); do
#     INIT_STR="$INIT_STR, 0"
# done
# echo "int ____num_tasks[32] = $INIT_STR };" >> $WITH_HEADER
cat $INPUT_PATH >> $WITH_HEADER

[[ $VERBOSE == 1 ]] && echo 'DEBUG >>> Inserting pragma markers'
cat $WITH_HEADER | python $REPLACE_PRAGMAS_WITH_FUNCTIONS true > $WITH_PRAGMA_MARKERS

TRANSFORMED=$DIRNAME/$FILE_PREFIX.$NAME.hclib.$EXTENSION
$TIME_BODY -o $TRANSFORMED $WITH_PRAGMA_MARKERS -- $INCLUDE $USER_INCLUDES $DEFINES -I$HCLIB_ROOT/include
cat $TRANSFORMED | grep -v "hclib_pragma_marker" > $OUTPUT_PATH

if [[ $KEEP == 0 ]]; then
    [[ $VERBOSE == 1 ]] && echo 'DEBUG >>> Cleaning up'
    rm -f $DIRNAME/$FILE_PREFIX.*
fi
