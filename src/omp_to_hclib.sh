#!/bin/bash

set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
BRACE_INSERT=$SCRIPT_DIR/brace_insert/brace_insert
INSERT_LINE_NO=$SCRIPT_DIR/insert_line_numbers.py
OPENMP_FINDER=$SCRIPT_DIR/openmp_finder.py
OMP_TO_HCLIB=$SCRIPT_DIR/omp_to_hclib/omp_to_hclib

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

WITH_PREPROC=$DIRNAME/$NAME.preproc.$EXTENSION
WITH_LINE_NO=$DIRNAME/$NAME.lines.$EXTENSION
WITH_BRACES=$DIRNAME/$NAME.braces.$EXTENSION
WITH_HCLIB=$DIRNAME/$NAME.hclib.$EXTENSION

OMP_INFO=$DIRNAME/$NAME.omp.info

# Preprocess the input file
$GXX -E $INPUT_PATH -o $WITH_PREPROC -g

# Find all uses of OpenMP pragrams in the input file and store them in $OMP_INFO
python $OPENMP_FINDER $WITH_PREPROC > $OMP_INFO

# Insert line number pragmas everywhere in the input line to ensure no
# transformations mess with that information
cat $WITH_PREPROC | python $INSERT_LINE_NO $INPUT_PATH > $WITH_LINE_NO

# Insert braces to simplify future transformatsion, i.e. to ensure block
# membership does not change because of inserted code
$BRACE_INSERT -o $WITH_BRACES $WITH_LINE_NO -- -I/usr/include -I/usr/include/linux

# Translate OMP pragmas detected by OPENMP_FINDER into HClib constructs
$OMP_TO_HCLIB -o $WITH_HCLIB -m $OMP_INFO $WITH_BRACES --  -I/usr/include -I/usr/include/linux

rm -f $WITH_PREPROC $WITH_LINE_NO $WITH_BRACES $OMP_INFO
