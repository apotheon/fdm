/* $Id: deliver-keep.c,v 1.1 2006-11-24 19:30:30 nicm Exp $ */

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

#include "fdm.h"

int	 keep_deliver(struct deliver_ctx *, struct action *);
char	*keep_desc(struct action *);

struct deliver deliver_keep = { DELIVER_INCHILD, keep_deliver, keep_desc };

int
keep_deliver(struct deliver_ctx *dctx, unused struct action *t)
{
	dctx->mail->decision = DECISION_KEEP;

	return (DELIVER_SUCCESS);
}

char *
keep_desc(unused struct action *t)
{
	return (xstrdup("keep"));
}
