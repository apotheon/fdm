#!/bin/sh
# $Id: test-macro3.sh,v 1.1 2007-08-30 22:06:08 nicm Exp $

. ./test.subr && test_init

cat <<EOF|test_in

EOF

cat <<EOF|test_out
file2
EOF

cat <<EOF|test_run
\$test="file1"
\$test="file2"
match all action rewrite "echo \${test}" continue
EOF
