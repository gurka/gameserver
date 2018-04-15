#!/bin/bash
set -e

# Go to repo root
SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
cd "$SCRIPT_DIR"/..

# Build 'debug' and run tests with valgrind
tools/cmake debug
pushd build/debug
make unittest
for test_bin in bin/*_test
do
  valgrind ./"$test_bin"
done
popd

# Build 'debug_asan* and run tests
tools/cmake debug_asan
pushd build/debug_asan
make unittest
for test_bin in bin/*_test
do
  ./"$test_bin"
done
popd

# Build 'release' and run tests
tools/cmake release
pushd build/release
make unittest
for test_bin in bin/*_test
do
  ./"$test_bin"
done
popd
