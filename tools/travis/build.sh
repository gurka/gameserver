#!/bin/bash
set -e

cd "$(git rev-parse --show-toplevel)"

STATUS=0

# Build debug
tools/cmake.sh debug
pushd build/debug
make || STATUS=1
popd

# Build debug-fast
tools/cmake.sh debug-fast
pushd build/debug-fast
make || STATUS=1
popd

# Build release
tools/cmake.sh release
pushd build/release
make || STATUS=1
popd

exit $STATUS
