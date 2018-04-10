#!/bin/bash
set -e

(
  DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
  pushd "$DIR/.."

  tools/cmake debug
  pushd "debug"
  ctest -V
)
