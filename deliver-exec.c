/* $Id: deliver-exec.c,v 1.2 2007-03-06 17:26:37 nicm Exp $ */

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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "fdm.h"
#include "deliver.h"

int	 deliver_exec_deliver(struct deliver_ctx *, struct action *);
void	 deliver_exec_desc(struct action *, char *, size_t);

struct deliver deliver_exec = {
	DELIVER_ASUSER,
	deliver_exec_deliver,
	deliver_exec_desc
};

int
deliver_exec_deliver(struct deliver_ctx *dctx, struct action *t)
{
	return (do_pipe(dctx, t, 0));
}

void
deliver_exec_desc(struct action *t, char *buf, size_t len)
{
	xsnprintf(buf, len, "exec \"%s\"", (char *) t->data);
}