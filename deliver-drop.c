/* $Id: deliver-drop.c,v 1.15 2007-03-06 17:26:37 nicm Exp $ */

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

int	 deliver_drop_deliver(struct deliver_ctx *, struct action *);
void	 deliver_drop_desc(struct action *, char *, size_t);

struct deliver deliver_drop = {
	DELIVER_INCHILD,
	deliver_drop_deliver,
	deliver_drop_desc
};

int
deliver_drop_deliver(struct deliver_ctx *dctx, unused struct action *t)
{
	*dctx->decision = DECISION_DROP;

	return (DELIVER_SUCCESS);
}

void
deliver_drop_desc(unused struct action *t, char *buf, size_t len)
{
	strlcpy(buf, "drop", len);
}
