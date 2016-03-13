#!/bin/bash

set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

mkdir -p $SCRIPT_DIR/test-output

FILES=$(find $SCRIPT_DIR/cpp/ -name "*.cpp")
FILES="$(find $SCRIPT_DIR/c/ -name "*.c") $FILES"

for FILE in $FILES; do
    DIRNAME=$(dirname $FILE)
    FILENAME=$(basename $FILE)
    EXTENSION="${FILENAME##*.}"
    NAME="${FILENAME%.*}"

    TEST_OUTPUT=$SCRIPT_DIR/test-output/$FILENAME
    REFERENCE=$(dirname $FILE)-ref/$FILENAME

    $SCRIPT_DIR/../src/omp_to_hclib.sh -i $FILE -o $TEST_OUTPUT $* &> transform.log

    if [[ ! -f $REFERENCE ]]; then
        echo
        echo Missing reference output for $FILE, exiting early
        echo Generated output is in $TEST_OUTPUT
        exit 1
    fi

    set +e
    diff $TEST_OUTPUT $REFERENCE > $SCRIPT_DIR/delta
    set -e

    LINES=$(cat $SCRIPT_DIR/delta | wc -l)
    if [[ $LINES != 0 ]]; then
        echo
        echo Non-empty delta for $FILE
        echo Delta is placed in $SCRIPT_DIR/delta
        echo Generated output is in $TEST_OUTPUT
        echo Reference output is in $REFERENCE
        exit 1
    fi

    rm $SCRIPT_DIR/delta transform.log
done

echo 'Passed all tests!'
