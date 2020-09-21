#!/bin/bash
set -xe

function release {
  mkdir -p "$BUILD_DIR/release"
  pushd "$BUILD_DIR/release"
  $CMAKE "$GAMESERVER_DIR" -G"Eclipse CDT4 - Unix Makefiles" -DCMAKE_ECLIPSE_VERSION=4.16 -DCMAKE_CXX_COMPILER_ARG1=-std=c++17 -DCMAKE_ECLIPSE_GENERATE_LINKED_RESOURCES=FALSE -DCMAKE_BUILD_TYPE=release
  popd
}

function debug {
  mkdir -p "$BUILD_DIR/debug"
  pushd "$BUILD_DIR/debug"
  $CMAKE "$GAMESERVER_DIR" -G"Eclipse CDT4 - Unix Makefiles" -DCMAKE_ECLIPSE_VERSION=4.16 -DCMAKE_CXX_COMPILER_ARG1=-std=c++17 -DCMAKE_ECLIPSE_GENERATE_LINKED_RESOURCES=FALSE -DCMAKE_BUILD_TYPE=debug
  popd
}

function debug-full {
  mkdir -p "$BUILD_DIR/debug-full"
  pushd "$BUILD_DIR/debug-full"
  $CMAKE "$GAMESERVER_DIR" -G"Eclipse CDT4 - Unix Makefiles" -DCMAKE_ECLIPSE_VERSION=4.16 -DCMAKE_CXX_COMPILER_ARG1=-std=c++17 -DCMAKE_ECLIPSE_GENERATE_LINKED_RESOURCES=FALSE -DCMAKE_BUILD_TYPE=debug -DGAMESERVER_DEBUG_FULL=ON
  popd
}

# Use clang and clang++, unless specified by the caller
export CC="${CC:=clang}"
export CXX="${CXX:=clang++}"

case $1 in
  'linux')
    CMAKE="cmake"
    ;;

  'emscripten')
    CMAKE="emcmake cmake"
    ;;

  *)
    echo "Usage: $0 [linux | emscripten] [all | release | debug | debug-full]"
    exit 1
    ;;
esac

ROOT_DIR=$(git rev-parse --show-toplevel)
BUILD_DIR="$ROOT_DIR/build/$1"
GAMESERVER_DIR="$ROOT_DIR/gameserver"

case $2 in
  'all')
    release
    debug
    debug-full
    ;;

  'release')
    release
    ;;

  'debug')
    debug
    ;;

  'debug-full')
    debug-full
    ;;

  *)
    echo "Usage: $0 [linux | emscripten] [all | release | debug | debug-full]"
    exit 1
    ;;
esac
