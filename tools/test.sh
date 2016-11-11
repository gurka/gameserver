#!/bin/bash

# Go to repo root
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
pushd "$DIR/.."

# Call tools/cmake to create build_test
tools/cmake test

# Go to build_test
pushd "build_test"
make || exit 1

# Run tests
./account_test || exit 1
./network_test || exit 1
./utils_test || exit 1
./world_test || exit 1

echo "All tests ran OK"
popd
popd
exit 0
