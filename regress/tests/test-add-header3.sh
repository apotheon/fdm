#!/bin/sh
# $Id: test-add-header3.sh,v 1.1 2007-08-30 21:46:33 nicm Exp $

. ./test.subr && test_init

cat <<EOF|test_in
Header: Test1

EOF

cat <<EOF|test_out
Header: Test1
Header: Test2

EOF

cat <<EOF|test_run
match all action add-header "Header" value "Test2" continue
EOF
