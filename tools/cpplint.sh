#!/bin/bash
set -e

cd "$(git rev-parse --show-toplevel)"

SOURCE_FILES=$(find "gameserver/" -name "*.cc" -or -name "*.h" | grep -v "/test/\|/test_export/")
CPPLINT_FILTER="-build/include,+build/include_order,+build/include_what_you_use,-whitespace/braces,-whitespace/newline,-readability/streams"
python "external/google-styleguide/cpplint/cpplint.py" --linelength=120 --filter="$CPPLINT_FILTER" --root=gameserver $SOURCE_FILES
