#!/bin/bash
set -e

cd "$(git rev-parse --show-toplevel)"

INCLUDE_DIRS=$(find gameserver/ -type d -name 'src' -or -name 'export' | grep -v '/test/' | sed 's/^/-I/')
IGNORE_DIRS=$(find gameserver/ -type d -name 'test' | sed 's/^/-i/')
cppcheck --std=c++11 --enable=all --error-exitcode=2 --quiet $IGNORE_DIRS $INCLUDE_DIRS gameserver/
