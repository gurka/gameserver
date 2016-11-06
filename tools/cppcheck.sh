#!/bin/bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cppcheck --std=c++11 --enable=all "$DIR/../gameserver"
