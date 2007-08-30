#!/bin/sh
# $Id: test-macro9.sh,v 1.1 2007-08-30 22:20:40 nicm Exp $

FDM="$FDM -D%test=1 -D\$test=argument"
. ./test.subr && test_init

cat <<EOF|test_in

EOF

cat <<EOF|test_out
1 argument
EOF

cat <<EOF|test_run
%test=3
\$test="file"
match all action rewrite "echo %{test} \${test}" continue
EOF
