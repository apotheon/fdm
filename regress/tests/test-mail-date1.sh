#!/bin/sh
# $Id: test-mail-date1.sh,v 1.1 2007-08-31 13:56:15 nicm Exp $

export TZ=GMT
. ./test.subr && test_init

cat <<EOF|test_in
Date: Thu, 30 Aug 2007 18:31:04 +0200

EOF

cat <<EOF|test_out
16 31 04 30 07 2007 Thu, 30 Aug 2007 16:31:04 +0000
EOF

cat <<EOF|test_run
\$date = "%[mail_hour] %[mail_minute] %[mail_second] " +
	"%[mail_day] %[mail_month] %[mail_year] %[mail_rfc822date]"
match all action rewrite "echo \${date}" continue
EOF
