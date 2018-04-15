#!/bin/bash

echo "Disabled"
exit 1
#SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
#INCLUDE_DIRS=$(find "$SCRIPT_DIR"/../gameserver/ -type d -name 'src' -or -name 'export' | grep -v '/test/' | sed 's/^/-I/')
#cppcheck --std=c++11 --enable=all $INCLUDE_DIRS "$SCRIPT_DIR"/../gameserver
