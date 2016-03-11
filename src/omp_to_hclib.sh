#!/bin/bash

set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
BRACE_INSERT=$SCRIPT_DIR/brace_insert/brace_insert
INSERT_LINE_NO=$SCRIPT_DIR/insert_line_numbers.py
OPENMP_FINDER=$SCRIPT_DIR/openmp_finder.py
OMP_TO_HCLIB=$SCRIPT_DIR/omp_to_hclib/omp_to_hclib
INSERT_STRUCTS=$SCRIPT_DIR/insert_structs.py

GXX="$GXX"
if [[ -z "$GXX" ]]; then
    GXX=g++
fi

if [[ $# != 1 ]]; then
    echo 'usage: omp_to_hclib.sh <file>'
    exit 1
fi

INPUT_PATH=$1

DIRNAME=$(dirname $INPUT_PATH)
FILENAME=$(basename $INPUT_PATH)
EXTENSION="${FILENAME##*.}"
NAME="${FILENAME%.*}"

INCLUDE=""

for DIR in $(cpp -v < /dev/null 2>&1 | awk 'BEGIN { doPrint = 0; } /#include <...> search starts here:/ { doPrint = 1; } /End of search list/ { doPrint = 0; } { if (doPrint) print $0; }' | tail -n +2); do
    INCLUDE="$INCLUDE -I$DIR"
done

echo $INCLUDE

WITH_BRACES=$DIRNAME/$NAME.braces.$EXTENSION
WITH_HCLIB=$DIRNAME/$NAME.hclib.$EXTENSION
WITH_STRUCTS=$DIRNAME/$NAME.structs.$EXTENSION

OMP_INFO=$DIRNAME/$NAME.omp.info
STRUCT_INFO=$DIRNAME/$NAME.struct.info

# Insert braces to simplify future transformatsion, i.e. to ensure block
# membership does not change because of inserted code
$BRACE_INSERT -o $WITH_BRACES $INPUT_PATH -- $INCLUDE

# Find all uses of OpenMP pragrams in the input file and store them in $OMP_INFO
python $OPENMP_FINDER $INPUT_PATH > $OMP_INFO

# Translate OMP pragmas detected by OPENMP_FINDER into HClib constructs
$OMP_TO_HCLIB -o $WITH_HCLIB -s $STRUCT_INFO -m $OMP_INFO $WITH_BRACES -- $INCLUDE

python $INSERT_STRUCTS $WITH_HCLIB $STRUCT_INFO > $WITH_STRUCTS

# rm -f $WITH_BRACES $OMP_INFO
