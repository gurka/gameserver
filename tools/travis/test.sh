#!/bin/bash
set -e

cd "$(git rev-parse --show-toplevel)"

STATUS=0

# Build 'travis_debug_asan* and run tests
tools/cmake.sh travis_debug_asan
pushd build/travis_debug_asan
make unittest || STATUS=1
for test_bin in bin/*_test
do
  ./"$test_bin" || STATUS=1
done
popd

# Build 'travis_debug' and run tests with valgrind
tools/cmake.sh travis_debug
pushd build/travis_debug
make unittest || STATUS=1
for test_bin in bin/*_test
do
  valgrind --error-exitcode=1 --leak-check=full ./"$test_bin" || STATUS=1
done
popd

exit $STATUS
