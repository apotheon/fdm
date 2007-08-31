#!/bin/sh
# $Id: test-matched1.sh,v 1.1 2007-08-31 13:41:56 nicm Exp $

. ./test.subr && test_init

cat <<EOF|test_in

EOF

cat <<EOF|test_out
test
EOF

cat <<EOF|test_run
match all action tag "tag" continue
match matched action rewrite "echo test" continue
EOF
