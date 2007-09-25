#!/bin/sh
# $Id: test-tag5.sh,v 1.2 2007-09-25 18:11:10 nicm Exp $

. ./test.subr && test_init

cat <<EOF|test_in
-abcdef-
EOF

cat <<EOF|test_out
--
EOF

cat <<EOF|test_run
set strip-characters "abcdef"
match "(.*)" action tag "test_tag" value "%1" continue
match all action rewrite "echo %[:test_tag]" continue
EOF
