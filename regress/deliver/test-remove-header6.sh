#!/bin/sh
# $Id: test-remove-header6.sh,v 1.1 2007-08-16 10:22:35 nicm Exp $

. ./test-deliver.subr && test_init

cat <<EOF|test_in
Header: Test
Header2: Test
Header: Test

EOF

cat <<EOF|test_out
Header: Test
Header2: Test
Header: Test

EOF

cat <<EOF|test_run
match all action remove-header "*Test*" continue
EOF
