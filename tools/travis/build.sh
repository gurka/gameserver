#!/bin/bash
set -e

cd "$(git rev-parse --show-toplevel)"

export CC=clang
export CXX=clang++
STATUS=0

# Server
tools/cmake.sh server all
pushd build/server/debug-full
make || STATUS=1
popd
pushd build/server/debug
make || STATUS=1
popd
pushd build/server/release
make || STATUS=1
popd

# Client
# TODO - emscripten in travis, or docker?

exit $STATUS
