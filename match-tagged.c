/* $Id: match-tagged.c,v 1.11 2007-02-09 15:40:20 nicm Exp $ */

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

int	tagged_match(struct match_ctx *, struct expritem *);
void	tagged_desc(struct expritem *, char *, size_t);

struct match match_tagged = { tagged_match, tagged_desc };

int
tagged_match(struct match_ctx *mctx, struct expritem *ei)
{
	struct tagged_data	*data = ei->data;
	struct mail		*m = mctx->mail;

	if (match_tag(&m->tags, data->tag) != NULL)
		return (MATCH_TRUE);
	return (MATCH_FALSE);
}

void
tagged_desc(struct expritem *ei, char *buf, size_t len)
{
	struct tagged_data	*data = ei->data;

	xsnprintf(buf, len, "tagged %s", data->tag);
}
