#!/bin/bash

set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

RUN_TESTS=$SCRIPT_DIR/run-tests.sh

set +e
$RUN_TESTS > tmp
set -e

PASSED=$(cat tmp | grep Passed | wc -l)
if [[ $PASSED -eq 1 ]]; then
    echo No failures!
    exit 0
fi

NEW_OUTPUT=$(cat tmp | tail -n 2 | head -n 1 | awk '{ print $5 }')
REF_LOCATION=$(cat tmp | tail -n 1 | awk '{ print $4 }')
rm -f tmp

echo Updating $REF_LOCATION
mkdir -p $(dirname $REF_LOCATION)

mv $NEW_OUTPUT $REF_LOCATION
