#!/bin/bash

set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

PLATFORM=$(uname)

function stat_file() {
    if [[ $PLATFORM == Darwin ]]; then
        stat -f '%d:%i' $1
    else
        stat -c '%d:%i' $1
    fi
}

function compare_outputs() {
    REFERENCE=$1
    FILE=$2
    TEST_OUTPUT=$3

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
}

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
    TIME_BODY_OUTPUT=$SCRIPT_DIR/test-output/$FILENAME.time_body
    TIME_BODY_REFERENCE=$(dirname $DIRNAME)-time-body-ref/$TESTNAME/$FILENAME

    CMD="$SCRIPT_DIR/../src/omp_to_hclib.sh -i $FILE -o $TEST_OUTPUT -I $DIRNAME -v"
    TIME_BODY_CMD="$SCRIPT_DIR/../src/time_body.sh -i $FILE -o $TIME_BODY_OUTPUT -I $DIRNAME -v"
    for P in "${!defines[@]}"; do
        if [[ "$(stat_file $P)" == "$(stat_file $FILE)" ]]; then
            for DEF in ${defines[$P]}; do
                CMD="$CMD -D $DEF"
                TIME_BODY_CMD="$TIME_BODY_CMD -D $DEF"
            done
        fi
    done

    $CMD &> transform.log
    compare_outputs $REFERENCE $FILE $TEST_OUTPUT

    $TIME_BODY_CMD &> transform.time_body.log
    compare_outputs $TIME_BODY_REFERENCE $FILE $TIME_BODY_OUTPUT

#     rm $SCRIPT_DIR/delta transform.log
done

echo 'Passed all tests!'
