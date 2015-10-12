#!/bin/bash
python `dirname $0`/google-styleguide/cpplint/cpplint.py --filter=-build/include,+build/include_order,+build/include_what_you_use,-whitespace/line_length,-whitespace/braces,-whitespace/newline,-readability/streams --root=src `find src/ | egrep "(.cc$|.h$)"`
