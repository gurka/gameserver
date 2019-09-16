#!/bin/bash
set -e

ROOT_DIR=$(git rev-parse --show-toplevel)
BUILD_DIR="$ROOT_DIR/build"
GAMESERVER_DIR="$ROOT_DIR/gameserver"
DATA_DIR="$ROOT_DIR/data"

function release {
  mkdir -p "$BUILD_DIR/release"
  pushd "$BUILD_DIR/release"
  cmake "$GAMESERVER_DIR" -DCMAKE_BUILD_TYPE=release
  ln -sf "$DATA_DIR" data
  popd
}

function debug {
  mkdir -p "$BUILD_DIR/debug"
  pushd "$BUILD_DIR/debug"
  cmake "$GAMESERVER_DIR" -DCMAKE_BUILD_TYPE=debug -DGAMESERVER_DEBUG_FULL=ON
  ln -sf "$DATA_DIR" data
  popd
}

function debug-fast {
  mkdir -p "$BUILD_DIR/debug-fast"
  pushd "$BUILD_DIR/debug-fast"
  cmake "$GAMESERVER_DIR" -DCMAKE_BUILD_TYPE=debug
  ln -sf "$DATA_DIR" data
  popd
}

# Same as debug, but for Eclipse CDT
function eclipse {
  mkdir -p "$BUILD_DIR/eclipse"
  pushd "$BUILD_DIR/eclipse"
  cmake "$GAMESERVER_DIR" -G"Eclipse CDT4 - Unix Makefiles" -DCMAKE_BUILD_TYPE=debug -DGAMESERVER_DEBUG_FULL=ON -DCMAKE_ECLIPSE_VERSION=4.12 -DCMAKE_CXX_COMPILER_ARG1=-std=c++17 -DCMAKE_ECLIPSE_GENERATE_LINKED_RESOURCES=FALSE
  ln -sf "$DATA_DIR" data
  popd
}

# Same as debug-fast, but for Eclipse CDT
function eclipse-fast {
  mkdir -p "$BUILD_DIR/eclipse-fast"
  pushd "$BUILD_DIR/eclipse-fast"
  cmake "$GAMESERVER_DIR" -G"Eclipse CDT4 - Unix Makefiles" -DCMAKE_BUILD_TYPE=debug -DCMAKE_ECLIPSE_VERSION=4.12 -DCMAKE_CXX_COMPILER_ARG1=-std=c++17 -DCMAKE_ECLIPSE_GENERATE_LINKED_RESOURCES=FALSE
  ln -sf "$DATA_DIR" data
  popd
}

function wsclient {
  mkdir -p "$BUILD_DIR/wsclient"
  pushd "$BUILD_DIR/wsclient"
  CC=emcc CXX=em++ cmake "$GAMESERVER_DIR" -G"Eclipse CDT4 - Unix Makefiles" -DCMAKE_BUILD_TYPE=debug -DGAMESERVER_WSCLIENT=ON -DGAMESERVER_DEBUG_FULL=ON -DCMAKE_ECLIPSE_VERSION=4.12 -DCMAKE_CXX_COMPILER_ARG1=-std=c++17 -DCMAKE_ECLIPSE_GENERATE_LINKED_RESOURCES=FALSE
  popd
}

function wsclient-fast {
  mkdir -p "$BUILD_DIR/wsclient-fast"
  pushd "$BUILD_DIR/wsclient-fast"
  CC=emcc CXX=em++ cmake "$GAMESERVER_DIR" -G"Eclipse CDT4 - Unix Makefiles" -DCMAKE_BUILD_TYPE=debug -DGAMESERVER_WSCLIENT=ON -DCMAKE_ECLIPSE_VERSION=4.12 -DCMAKE_CXX_COMPILER_ARG1=-std=c++17 -DCMAKE_ECLIPSE_GENERATE_LINKED_RESOURCES=FALSE
  popd
}

case $1 in
  'all')
    release
    debug
    debug-fast
    eclipse
    wsclient
    wsclient-fast
    ;;

  'release')
    release
    ;;

  'debug')
    debug
    ;;

  'debug-fast')
    debug-fast
    ;;

  'eclipse')
    eclipse
    ;;

  'eclipse-fast')
    eclipse-fast
    ;;

  'wsclient')
    wsclient
    ;;

  'wsclient-fast')
    wsclient-fast
    ;;

  *)
    echo "Usage: $0 [all | release | debug | debug-fast | eclipse | eclipse-fast | wsclient | wsclient-fast]"
    ;;
esac
