#!/bin/sh
# $Id: test-add-header1.sh,v 1.1 2007-08-30 21:46:33 nicm Exp $

. ./test.subr && test_init

cat <<EOF|test_in

EOF

cat <<EOF|test_out
Header: Test

EOF

cat <<EOF|test_run
match all action add-header "Header" value "Test" continue
EOF
