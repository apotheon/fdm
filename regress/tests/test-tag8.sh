#!/bin/sh
# $Id: test-tag8.sh,v 1.1 2007-09-25 18:11:10 nicm Exp $

. ./test.subr && test_init

cat <<EOF|test_in

EOF

cat <<EOF|test_out

EOF

cat <<EOF|test_run
match all action tag "test_tag" value "" continue
match all action rewrite "echo %[test_tag]" continue
EOF
