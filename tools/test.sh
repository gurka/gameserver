#!/bin/bash

# Go to script directory
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
pushd "$DIR/.."

# Create build_test (may already exist)
mkdir -p build_test
cd build_test

# Run cmake and make
cmake .. -DCMAKE_BUILD_TYPE=debug -Dunittest=ON || exit 1
make || exit 1

# Run tests
./account_test || exit 1
./network_test || exit 1
./utils_test || exit 1
./world_test || exit 1

echo "All tests ran OK"
popd
exit 0
