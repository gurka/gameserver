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
  cmake "$GAMESERVER_DIR" -DCMAKE_BUILD_TYPE=debug
  ln -sf "$DATA_DIR" data
  popd
}

# Same as debug, but with ASAN
function debug_asan {
  mkdir -p "$BUILD_DIR/debug_asan"
  pushd "$BUILD_DIR/debug_asan"
  cmake "$GAMESERVER_DIR" -DCMAKE_BUILD_TYPE=debug -DGAMESERVER_USE_ASAN=ON
  ln -sf "$DATA_DIR" data
  popd
}

# Same as debug, but with ld=gold for travis
function travis_debug {
  mkdir -p "$BUILD_DIR/travis_debug"
  pushd "$BUILD_DIR/travis_debug"
  cmake "$GAMESERVER_DIR" -DCMAKE_BUILD_TYPE=debug -DGAMESERVER_USE_LD_GOLD=ON
  ln -sf "$DATA_DIR" data
  popd
}

# Same as debug_asan, but with ld=gold for travis
function travis_debug_asan {
  mkdir -p "$BUILD_DIR/travis_debug_asan"
  pushd "$BUILD_DIR/travis_debug_asan"
  cmake "$GAMESERVER_DIR" -DCMAKE_BUILD_TYPE=debug -DGAMESERVER_USE_ASAN=ON -DGAMESERVER_USE_LD_GOLD=ON
  ln -sf "$DATA_DIR" data
  popd
}

# Same as debug_asan, but for Eclipse CDT
function eclipse {
  mkdir -p "$BUILD_DIR/eclipse"
  pushd "$BUILD_DIR/eclipse"
  cmake "$GAMESERVER_DIR" -G"Eclipse CDT4 - Unix Makefiles" -DCMAKE_BUILD_TYPE=debug -DGAMESERVER_USE_ASAN=ON -DCMAKE_ECLIPSE_VERSION=4.12 -DCMAKE_CXX_COMPILER_ARG1=-std=c++14
  ln -sf "$DATA_DIR" data
  popd
}

case $1 in
  'all')
    release
    debug
    debug_asan
    travis_debug
    travis_debug_asan
    eclipse
    ;;

  'release')
    release
    ;;

  'debug')
    debug
    ;;

  'debug_asan')
    debug_asan
    ;;

  'travis_debug')
    travis_debug
    ;;

  'travis_debug_asan')
    travis_debug_asan
    ;;

  'eclipse')
    eclipse
    ;;

  *)
    echo "Usage: $0 [all | release | debug | debug_asan | travis_debug | travis_debug_asan | eclipse]"
    ;;
esac
