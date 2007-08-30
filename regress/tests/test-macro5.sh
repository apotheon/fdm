#!/bin/sh
# $Id: test-macro5.sh,v 1.1 2007-08-30 22:20:40 nicm Exp $

FDM="$FDM -D%test=1"
. ./test.subr && test_init

cat <<EOF|test_in

EOF

cat <<EOF|test_out
1
EOF

cat <<EOF|test_run
%test=2
match all action rewrite "echo %{test}" continue
EOF
