#!/bin/bash
set -e

cd "$(git rev-parse --show-toplevel)"

STATUS=0

# Build 'debug' and run tests
tools/cmake.sh debug
pushd build/debug
make unittest || STATUS=1
for test_bin in bin/*_test
do
  ./"$test_bin" || STATUS=1
done
popd

# Build 'debug-fast' and run tests with valgrind
tools/cmake.sh debug-fast
pushd build/debug-fast
make unittest || STATUS=1
for test_bin in bin/*_test
do
  valgrind --error-exitcode=1 --leak-check=full ./"$test_bin" || STATUS=1
done
popd

exit $STATUS
