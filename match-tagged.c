/* $Id: match-tagged.c,v 1.19 2007-03-19 20:04:48 nicm Exp $ */

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

#include <fnmatch.h>
#include <string.h>

#include "fdm.h"
#include "match.h"

int	match_tagged_match(struct mail_ctx *, struct expritem *);
void	match_tagged_desc(struct expritem *, char *, size_t);

struct match match_tagged = {
	"tagged",
	match_tagged_match,
	match_tagged_desc
};

int
match_tagged_match(struct mail_ctx *mctx, struct expritem *ei)
{
	struct match_tagged_data	*data = ei->data;
	struct mail			*m = mctx->mail;
	char				*tag;

	tag = replacestr(&data->tag, m->tags, m, &m->rml);
	if (match_tag(m->tags, tag) != NULL) {
		xfree(tag);
		return (MATCH_TRUE);
	}
	xfree(tag);
	return (MATCH_FALSE);
}

void
match_tagged_desc(struct expritem *ei, char *buf, size_t len)
{
	struct match_tagged_data	*data = ei->data;

	xsnprintf(buf, len, "tagged %s", data->tag.str);
}
