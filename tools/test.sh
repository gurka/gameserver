#!/bin/bash
set -e

(
  DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
  pushd "$DIR/.."

  tools/cmake debug
  pushd "debug"
  make tests

  bin/account_test
  bin/network_test
  bin/utils_test
  bin/world_test

  echo "All tests ran OK"
)
