/* $Id: match-unmatched.c,v 1.3 2006-11-25 11:55:07 nicm Exp $ */

/*
 * Copyright (c) 2006 Nicholas Marriott <nicm@users.sourceforge.net>
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

#include <sys/types.h>

#include <string.h>

#include "fdm.h"

int	unmatched_match(struct match_ctx *, struct expritem *);
char   *unmatched_desc(struct expritem *);

struct match match_unmatched = { unmatched_match, unmatched_desc };

int
unmatched_match(struct match_ctx *mctx, unused struct expritem *ei)
{
	if (*mctx->matched)
		return (MATCH_FALSE);
	return (MATCH_TRUE);
}

char *
unmatched_desc(unused struct expritem *ei)
{
	return (xstrdup("unmatched"));
}
