#!/bin/bash
set -e

# Go to repo root
SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
cd "$SCRIPT_DIR"/../..

# Build 'travis_debug_asan* and run tests
tools/cmake travis_debug_asan
pushd build/travis_debug_asan
make unittest
for test_bin in bin/*_test
do
  ./"$test_bin"
done
popd
