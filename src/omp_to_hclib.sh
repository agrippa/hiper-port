#!/bin/bash

set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

BRACE_INSERT=$SCRIPT_DIR/brace_insert/brace_insert
INSERT_LINE_NO=$SCRIPT_DIR/insert_line_numbers.py
OPENMP_FINDER=$SCRIPT_DIR/openmp_finder.py
OMP_TO_HCLIB=$SCRIPT_DIR/omp_to_hclib/omp_to_hclib
INSERT_STRUCTS=$SCRIPT_DIR/insert_structs.py
REMOVE_OMP_PRAGMAS_AND_INSERT_HEADER=$SCRIPT_DIR/remove_omp_pragmas_and_insert_header.py 

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

INCLUDE=""

for DIR in $(cpp -v < /dev/null 2>&1 | awk 'BEGIN { doPrint = 0; } /#include <...> search starts here:/ { doPrint = 1; } /End of search list/ { doPrint = 0; } { if (doPrint) print $0; }' | tail -n +2); do
    INCLUDE="$INCLUDE -I$DIR"
done

echo $INCLUDE

WITH_BRACES=$DIRNAME/____omp_to_hclib.$NAME.braces.$EXTENSION
WITH_HCLIB=$DIRNAME/____omp_to_hclib.$NAME.hclib.$EXTENSION
WITHOUT_PRAGMAS=$DIRNAME/____omp_to_hclib.$NAME.no_pragmas.$EXTENSION

OMP_INFO=$DIRNAME/$NAME.omp.info

# Insert braces to simplify future transformatsion, i.e. to ensure block
# membership does not change because of inserted code
[[ $VERBOSE == 1 ]] && echo 'DEBUG >>> Inserting braces'
$BRACE_INSERT -o $WITH_BRACES $INPUT_PATH -- $INCLUDE $USER_INCLUDES

# Find all uses of OpenMP pragrams in the input file and store them in $OMP_INFO
[[ $VERBOSE == 1 ]] && echo 'DEBUG >>> Finding OMP pragmas'
python $OPENMP_FINDER $WITH_BRACES > $OMP_INFO

# Translate OMP pragmas detected by OPENMP_FINDER into HClib constructs
[[ $VERBOSE == 1 ]] && echo 'DEBUG >>> Converting OMP parallelism to HClib'
$OMP_TO_HCLIB -o $WITH_HCLIB -m $OMP_INFO $WITH_BRACES -- $INCLUDE $USER_INCLUDES

# Remove any OMP pragmas
[[ $VERBOSE == 1 ]] && echo 'DEBUG >>> Removing OMP pragmas'
cat $WITH_HCLIB | python $REMOVE_OMP_PRAGMAS_AND_INSERT_HEADER > $WITHOUT_PRAGMAS

[[ $VERBOSE == 1 ]] && echo 'DEBUG >>> Producing final output file'
cp $WITHOUT_PRAGMAS $OUTPUT_PATH

if [[ $KEEP == 0 ]]; then
    [[ $VERBOSE == 1 ]] && echo 'DEBUG >>> Cleaning up'
    rm -f $WITH_BRACES $OMP_INFO $WITH_HCLIB $WITHOUT_PRAGMAS
fi
