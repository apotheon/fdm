#!/bin/sh
# $Id: test-unmatched1.sh,v 1.1 2007-08-31 13:41:56 nicm Exp $

. ./test.subr && test_init

cat <<EOF|test_in

EOF

cat <<EOF|test_out

EOF

cat <<EOF|test_run
match all action tag "tag" continue
match unmatched action rewrite "echo test" continue
EOF
