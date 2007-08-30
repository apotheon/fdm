#!/bin/sh
# $Id: test-remove-header5.sh,v 1.1 2007-08-30 21:46:34 nicm Exp $

. ./test.subr && test_init

cat <<EOF|test_in
Header: Test
Header2: Test
Header: Test

EOF

cat <<EOF|test_out

EOF

cat <<EOF|test_run
match all action remove-header "Header*" continue
EOF
