#!/bin/sh
# $Id: test-remove-header7.sh,v 1.1 2007-08-24 09:46:09 nicm Exp $

. ./test-deliver.subr && test_init

cat <<EOF|test_in
Header: Test
Header2: Test
Header: Test

EOF

cat <<EOF|test_out

EOF

cat <<EOF|test_run
match all action remove-headers { "Header" "Header2" } continue
EOF
