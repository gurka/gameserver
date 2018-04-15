#!/bin/bash
set -e

cd "$(git rev-parse --show-toplevel)"

# Build 'travis_debug_asan* and run tests
tools/cmake travis_debug_asan
pushd build/travis_debug_asan
make unittest
for test_bin in bin/*_test
do
  ./"$test_bin"
done
popd
