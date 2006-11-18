/* $Id: match-tagged.c,v 1.1 2006-11-18 17:03:35 nicm Exp $ */

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

int	tagged_match(struct account *, struct mail *, struct expritem *);
char   *tagged_desc(struct expritem *);

struct match match_tagged = { "tagged", tagged_match, tagged_desc };

int
tagged_match(unused struct account *a, struct mail *m, struct expritem *ei)
{
	struct tagged_data	*data;
	u_int			 i;

	data = ei->data;
	
	for (i = 0; i < ARRAY_LENGTH(&m->tags); i++) {
		if (strcmp(data->tag, ARRAY_ITEM(&m->tags, i, char *)) == 0)
			return (1);
	}
	
	return (0);
}

char *
tagged_desc(struct expritem *ei)
{
	struct tagged_data	*data;

	data = ei->data;

	return (xstrdup(data->tag));
}
