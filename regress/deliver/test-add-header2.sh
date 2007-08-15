#!/bin/sh
# $Id: test-add-header2.sh,v 1.1 2007-08-15 23:38:58 nicm Exp $

. ./test-deliver.subr && test_init

cat <<EOF|test_in
Body
EOF

cat <<EOF|test_out
Header: Test

Body
EOF

cat <<EOF|test_run
match all action add-header "Header" value "Test" continue
EOF
