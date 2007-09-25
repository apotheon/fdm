#!/bin/sh
# $Id: test-tag7.sh,v 1.1 2007-09-25 18:11:10 nicm Exp $

. ./test.subr && test_init

cat <<EOF|test_in
-abcdef-
EOF

cat <<EOF|test_out
-abcdef-
EOF

cat <<EOF|test_run
set strip-characters ""
match "(.*)" action tag "test_tag" value "%1" continue
match all action rewrite "echo %[:test_tag]" continue
EOF
