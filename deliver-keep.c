/* $Id: deliver-keep.c,v 1.7 2007-03-21 22:49:44 nicm Exp $ */

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
#include "deliver.h"

int	 deliver_keep_deliver(struct deliver_ctx *, struct action *);
void	 deliver_keep_desc(struct action *, char *, size_t);

struct deliver deliver_keep = {
	"keep",
	DELIVER_INCHILD,
	deliver_keep_deliver,
	deliver_keep_desc
};

int
deliver_keep_deliver(struct deliver_ctx *dctx, unused struct action *t)
{
	struct mail	*m = dctx->mail;

	m->decision = DECISION_KEEP;

	return (DELIVER_SUCCESS);
}

void
deliver_keep_desc(unused struct action *t, char *buf, size_t len)
{
	strlcpy(buf, "keep", len);
}
