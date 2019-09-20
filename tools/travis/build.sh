#!/bin/bash
set -e

cd "$(git rev-parse --show-toplevel)"

export CC=clang
export CXX=clang++
STATUS=0

# Build debug
tools/cmake.sh debug
pushd build/debug
make || STATUS=1
popd

# Build debug-full
tools/cmake.sh debug-full
pushd build/debug-full
make || STATUS=1
popd

# Build release
tools/cmake.sh release
pushd build/release
make || STATUS=1
popd

exit $STATUS
