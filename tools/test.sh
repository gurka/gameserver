#!/bin/bash
set -e

(
  # Go to repo root
  DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
  pushd "$DIR/.."

  # Call tools/cmake to create build_test
  tools/cmake test

  # Go to build_test
  pushd "build_test"
  make || exit 1

  # Run tests
  bin/account_test
  bin/network_test
  bin/utils_test
  bin/world_test

  echo "All tests ran OK"
  exit 0
)
