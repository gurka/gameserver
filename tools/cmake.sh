#!/bin/bash
set -xe

ROOT_DIR=$(git rev-parse --show-toplevel)
BUILD_DIR="$ROOT_DIR/build"
GAMESERVER_DIR="$ROOT_DIR/gameserver"
DATA_DIR="$ROOT_DIR/data"


function release {
  mkdir -p "$BUILD_DIR/release"
  pushd "$BUILD_DIR/release"
  cmake "$GAMESERVER_DIR" -G"Eclipse CDT4 - Unix Makefiles" -DCMAKE_ECLIPSE_VERSION=4.12 -DCMAKE_CXX_COMPILER_ARG1=-std=c++17 -DCMAKE_ECLIPSE_GENERATE_LINKED_RESOURCES=FALSE -DCMAKE_BUILD_TYPE=release
  ln -sfn "$DATA_DIR" data
  popd
}

function debug {
  mkdir -p "$BUILD_DIR/debug"
  pushd "$BUILD_DIR/debug"
  cmake "$GAMESERVER_DIR" -G"Eclipse CDT4 - Unix Makefiles" -DCMAKE_ECLIPSE_VERSION=4.12 -DCMAKE_CXX_COMPILER_ARG1=-std=c++17 -DCMAKE_ECLIPSE_GENERATE_LINKED_RESOURCES=FALSE -DCMAKE_BUILD_TYPE=debug
  ln -sfn "$DATA_DIR" data
  popd
}

function debug-full {
  mkdir -p "$BUILD_DIR/debug-full"
  pushd "$BUILD_DIR/debug-full"
  cmake "$GAMESERVER_DIR" -G"Eclipse CDT4 - Unix Makefiles" -DCMAKE_ECLIPSE_VERSION=4.12 -DCMAKE_CXX_COMPILER_ARG1=-std=c++17 -DCMAKE_ECLIPSE_GENERATE_LINKED_RESOURCES=FALSE -DCMAKE_BUILD_TYPE=debug -DGAMESERVER_DEBUG_FULL=ON
  ln -sfn "$DATA_DIR" data
  popd
}

function wsclient {
  mkdir -p "$BUILD_DIR/wsclient"
  pushd "$BUILD_DIR/wsclient"
  ln -sfn "$DATA_DIR" data
  CC=emcc CXX=em++ cmake "$GAMESERVER_DIR/wsclient" -G"Eclipse CDT4 - Unix Makefiles" -DCMAKE_ECLIPSE_VERSION=4.12 -DCMAKE_CXX_COMPILER_ARG1=-std=c++17 -DCMAKE_ECLIPSE_GENERATE_LINKED_RESOURCES=FALSE -DCMAKE_BUILD_TYPE=debug
  popd
}

# Use clang and clang++, unless specified by the caller
CC="${CC:=clang}"
CXX="${CXX:=clang++}"
export CC
export CXX

case $1 in
  'all')
    release
    debug
    debug-full
    wsclient
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

  'wsclient')
    wsclient
    ;;

  *)
    echo "Usage: $0 [all | release | debug | debug-full | wsclient]"
    ;;
esac
