#!/bin/bash

set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

mkdir -p $SCRIPT_DIR/test-output

FILES=$(find $SCRIPT_DIR/cpp/ -name "*.cpp")
FILES="$(find $SCRIPT_DIR/c/ -name "*.c") $FILES"

if [[ $# == 1 ]]; then
    if [[ ! -f $1 ]]; then
        echo Missing test file $1
        exit 1
    fi
    FILES=$1
fi

for FILE in $FILES; do
    echo Processing $FILE
    DIRNAME=$(dirname $FILE)
    FILENAME=$(basename $FILE)

    TEST_OUTPUT=$SCRIPT_DIR/test-output/$FILENAME
    REFERENCE=$(dirname $DIRNAME)-ref/$FILENAME

    $SCRIPT_DIR/../src/omp_to_hclib.sh -i $FILE -o $TEST_OUTPUT -I $DIRNAME $* &> transform.log

    if [[ ! -f $REFERENCE ]]; then
        echo
        echo Missing reference output at $REFERENCE for $FILE, exiting early
        echo Generated output is in $TEST_OUTPUT
        exit 1
    fi

    set +e
    diff $REFERENCE $TEST_OUTPUT > $SCRIPT_DIR/delta
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

#     rm $SCRIPT_DIR/delta transform.log
done

echo 'Passed all tests!'
