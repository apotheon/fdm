#!/bin/sh
# $Id: test-macro6.sh,v 1.1 2007-08-30 22:20:40 nicm Exp $

. ./test.subr && test_init

cat <<EOF|test_in

EOF

cat <<EOF|test_out
1
EOF

cat <<EOF|test_run
%test=1
match all action rewrite "echo %{test}" continue
EOF
