#!/bin/bash

set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

RUN_TESTS=$SCRIPT_DIR/run-tests.sh

$RUN_TESTS > tmp
NEW_OUTPUT=$(cat tmp | tail -n 2 | head -n 1 | awk '{ print $5 }')
REF_LOCATION=$(cat tmp | tail -n 1 | awk '{ print $5 }')
rm -f tmp

mv $NEW_OUTPUT $REF_LOCATION
