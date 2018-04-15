#!/bin/bash
set -e

cd "$(git rev-parse --show-toplevel)"

SOURCE_FILES=$(find "gameserver/" -name "*.cc" -or -name "*.h" | grep -v "/test/\|/test_export/")
CPPLINT_FILTER="-build/include,+build/include_order,+build/include_what_you_use,-whitespace/line_length,-whitespace/braces,-whitespace/newline,-readability/streams,-build/header_guard"
python "external/google-styleguide/cpplint/cpplint.py" --filter="$CPPLINT_FILTER" --root=gameserver $SOURCE_FILES
