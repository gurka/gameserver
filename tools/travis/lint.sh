#!/bin/bash
set -x

cd "$(git rev-parse --show-toplevel)"

STATUS=0

# Run cpplint
CPPLINT_SOURCE_FILES=$(find "gameserver/" -name "*.cc" -or -name "*.h" | grep -v "/test/\|/test_export/")
CPPLINT_FILTER="-build/include,+build/include_order,+build/include_what_you_use,-whitespace/braces,-whitespace/newline"
python "external/google-styleguide/cpplint/cpplint.py" --linelength=120 --filter="$CPPLINT_FILTER" --root=gameserver $CPPLINT_SOURCE_FILES || STATUS=1

# Run cppcheck but ignore exit code for now
CPPCHECK_INCLUDE_DIRS=$(find gameserver/ -type d -name 'src' -or -name 'export' | grep -v '/test/' | sed 's/^/-I/')
CPPCHECK_IGNORE_DIRS=$(find gameserver/ -type d -name 'test' | sed 's/^/-i/')
cppcheck --std=c++11 --enable=all --error-exitcode=2 --quiet $CPPCHECK_IGNORE_DIRS $CPPCHECK_INCLUDE_DIRS gameserver/

exit $STATUS
