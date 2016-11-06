#!/bin/bash
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
FILES=`find "$DIR/../gameserver/" -name "*.cc" -or -name "*.h" | grep -v "/test/\|/test_export/"`
FILTER="-build/include,+build/include_order,+build/include_what_you_use,-whitespace/line_length,-whitespace/braces,-whitespace/newline,-readability/streams,-build/header_guard"
python "$DIR/google-styleguide/cpplint/cpplint.py" --filter=$FILTER --root=gameserver $FILES
