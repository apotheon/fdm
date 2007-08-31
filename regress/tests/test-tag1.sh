#!/bin/sh
# $Id: test-tag1.sh,v 1.1 2007-08-31 11:21:09 nicm Exp $

. ./test.subr && test_init

cat <<EOF|test_in

EOF

cat <<EOF|test_out
tag
EOF

cat <<EOF|test_run
match all action {
	tag "test_tag" value "tag"
	rewrite "echo %[test_tag]"
} continue
EOF
