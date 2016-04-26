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

declare -A defines
defines['cpp/uts-shmem-omp/uts_omp_task_shmem.cpp']='BRG_RNG _OPENMP'
defines['cpp/uts-omp/uts_omp_task.cpp']='BRG_RNG _OPENMP'

for FILE in $FILES; do
    DIRNAME=$(dirname $FILE)
    FILENAME=$(basename $FILE)
    TESTNAME=$(basename $(dirname $FILE))

    echo Running $TESTNAME \($FILE\)

    TEST_OUTPUT=$SCRIPT_DIR/test-output/$FILENAME
    REFERENCE=$(dirname $DIRNAME)-ref/$TESTNAME/$FILENAME

    CMD="$SCRIPT_DIR/../src/omp_to_hclib.sh -i $FILE -o $TEST_OUTPUT -I $DIRNAME -v"
    for P in "${!defines[@]}"; do
        if [[ "$(stat -c '%d:%i' $P)" == "$(stat -c '%d:%i' $FILE)" ]]; then
            for DEF in ${defines[$P]}; do
                CMD="$CMD -D $DEF"
            done
        fi
    done
    $CMD &> transform.log

    if [[ ! -f $REFERENCE ]]; then
        echo
        echo Missing reference output at $REFERENCE for $FILE, exiting early
        echo Generated output is in $TEST_OUTPUT
        echo Reference goes at $REFERENCE
        exit 1
    fi

    set +e
    diff $REFERENCE $TEST_OUTPUT > $SCRIPT_DIR/delta
    set -e

    LINES=$(cat $SCRIPT_DIR/delta | wc -l)
    if [[ $LINES -ne 0 ]]; then
        echo
        echo Non-empty delta for $FILE
        echo Delta is placed in $SCRIPT_DIR/delta
        echo Generated output is in $TEST_OUTPUT
        echo Reference goes at $REFERENCE
        exit 1
    fi

#     rm $SCRIPT_DIR/delta transform.log
done

echo 'Passed all tests!'
