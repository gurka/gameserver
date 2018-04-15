#!/bin/bash
set -e

cd "$(git rev-parse --show-toplevel)"

# Build 'travis_debug' and run tests with valgrind
tools/cmake travis_debug
pushd build/travis_debug
make unittest
for test_bin in bin/*_test
do
  valgrind ./"$test_bin"
done
popd
