#!/bin/bash
set -e

cd "$(git rev-parse --show-toplevel)"

export CC=clang
export CXX=clang++
STATUS=0

# linux
tools/cmake.sh linux all
pushd build/linux/debug-full
make || STATUS=1
popd
pushd build/linux/debug
make || STATUS=1
popd
pushd build/linux/release
make || STATUS=1
popd

# emscripten
# TODO

exit $STATUS
