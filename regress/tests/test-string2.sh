#!/bin/sh
# $Id: test-string2.sh,v 1.3 2007-09-24 20:46:13 nicm Exp $

. ./test.subr && test_init

cat <<EOF|test_in
Message-Id: test

EOF

cat <<EOF|test_out
test
EOF

cat <<EOF|test_run
match not string "%[message_id]" 
	to case "T..T" action rewrite "echo test" continue
EOF
