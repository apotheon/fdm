#!/bin/sh
# $Id: test-macro4.sh,v 1.1 2007-08-30 22:06:08 nicm Exp $

FDM="$FDM -D\$test1=argument -D\$test2=argument"
. ./test.subr && test_init

cat <<EOF|test_in

EOF

cat <<EOF|test_out
argument argument
EOF

cat <<EOF|test_run
\$test1="file"
\$test2="file"
match all action rewrite "echo \${test1} \${test2}" continue
EOF
