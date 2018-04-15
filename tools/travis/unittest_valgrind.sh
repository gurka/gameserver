#!/bin/bash
set -e

# Go to repo root
SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
cd "$SCRIPT_DIR"/../..

# Build 'travis_debug' and run tests with valgrind
tools/cmake travis_debug
pushd build/travis_debug
make unittest
for test_bin in bin/*_test
do
  valgrind ./"$test_bin"
done
popd
