/* $Id: match.h,v 1.16 2008-06-26 18:41:00 nicm Exp $ */

/*
 * Copyright (c) 2007 Nicholas Marriott <nicm@users.sourceforge.net>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF MIND, USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef MATCH_H
#define MATCH_H

#include "deliver.h"

/* Match return codes. */
#define MATCH_FALSE 0
#define MATCH_TRUE 1
#define MATCH_ERROR 2
#define MATCH_PARENT 3

/* Match functions. */
struct match {
	const char	*name;

	int		 (*match)(struct mail_ctx *, struct expritem *);
	void 		 (*desc)(struct expritem *, char *, size_t);
};

/* Match attachment data. */
struct match_attachment_data {
	enum {
		ATTACHOP_COUNT,
		ATTACHOP_TOTALSIZE,
		ATTACHOP_ANYSIZE,
		ATTACHOP_ANYTYPE,
		ATTACHOP_ANYNAME
	} op;

	enum cmp	 	 cmp;
	union {
		size_t		 size;
		long long	 num;
		struct replstr	 str;
		struct re	 re;
	} value;
};

/* Match account data. */
struct match_account_data {
	struct replstrs	*accounts;
};

/* Match age data. */
struct match_age_data {
	long long	 time;
	enum cmp	 cmp;
};

/* Match size data. */
struct match_size_data {
	size_t		 size;
	enum cmp	 cmp;
};

/* Match tagged data. */
struct match_tagged_data {
	struct replstr	 tag;
};

/* Match string data. */
struct match_string_data {
	struct replstr	 str;
	struct re 	 re;
};

/* Match regexp data. */
struct match_regexp_data {
	struct re	 re;

	enum area 	 area;
};

/* Match command data. */
struct match_command_data {
	struct replpath	 cmd;
	char		*user;
	int		 pipe;		/* pipe mail to command */

	struct re	 re;		/* re->str NULL to not check */
	int		 ret;		/* -1 to not check */
};

/* Match in-cache data. */
struct match_in_cache_data {
	char		*path;
	struct replstr	 key;
};

/* match-age.c */
extern struct match	 match_age;

/* match-all.c */
extern struct match	 match_all;

/* match-account.c */
extern struct match	 match_account;

/* match-attachment.c */
extern struct match	 match_attachment;

/* match-matched.c */
extern struct match	 match_matched;

/* match-unmatched.c */
extern struct match	 match_unmatched;

/* match-size.c */
extern struct match	 match_size;

/* match-tagged.c */
extern struct match	 match_tagged;

/* match-string.c */
extern struct match	 match_string;

/* match-command.c */
extern struct match	 match_command;

/* match-regexp.c */
extern struct match	 match_regexp;

/* match-in-cache.c */
extern struct match	 match_in_cache;

#endif
