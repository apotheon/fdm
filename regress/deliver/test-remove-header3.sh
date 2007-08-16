#!/bin/sh
# $Id: test-remove-header3.sh,v 1.1 2007-08-16 08:44:46 nicm Exp $

. ./test-deliver.subr && test_init

cat <<EOF|test_in
Header: Test
Header: Test

EOF

cat <<EOF|test_out

EOF

cat <<EOF|test_run
match all action remove-header "Header" continue
EOF
