#!/bin/bash
set -e

cd "$(git rev-parse --show-toplevel)"

STATUS=0

# Build 'debug-full' and run tests
tools/cmake.sh debug-full
pushd build/debug-full
make unittest || STATUS=1
for test_bin in bin/*_test
do
  ./"$test_bin" || STATUS=1
done
popd

# Build 'debug' and run tests with valgrind
tools/cmake.sh debug
pushd build/debug
make unittest || STATUS=1
for test_bin in bin/*_test
do
  valgrind --error-exitcode=1 --leak-check=full ./"$test_bin" || STATUS=1
done
popd

exit $STATUS
