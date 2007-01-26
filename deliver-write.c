/* $Id: deliver-write.c,v 1.17 2007-01-26 19:47:21 nicm Exp $ */

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

int	 write_deliver(struct deliver_ctx *, struct action *);
void	 write_desc(struct action *, char *, size_t);

struct deliver deliver_write = { DELIVER_ASUSER, write_deliver, write_desc };

int
write_deliver(struct deliver_ctx *dctx, struct action *t)
{
	return (do_write(dctx, t, 0));
}

void
write_desc(struct action *t, char *buf, size_t len)
{
	if (snprintf(buf, len, "write \"%s\"", (char *) t->data) == -1)
		fatal("snprintf");
}

int
do_write(struct deliver_ctx *dctx, struct action *t, int append)
{
	struct account	*a = dctx->account;
	struct mail	*m = dctx->mail;
        char		*path;
        FILE    	*f;

	path = replacepmatch(t->data, a, t, m->src, m, dctx->pmatch_valid,
	    dctx->pmatch);
        if (path == NULL || *path == '\0') {
		if (path != NULL)
			xfree(path);
		log_warnx("%s: empty command", a->name);
                return (DELIVER_FAILURE);
        }

	if (append)
		log_debug("%s: appending to %s", a->name, path);
	else
		log_debug("%s: writing to %s", a->name, path);
        f = fopen(path, append ? "a" : "w");
        if (f == NULL) {
		log_warn("%s: %s: fopen", a->name, path);
		xfree(path);
		return (DELIVER_FAILURE);
	}
	if (fwrite(m->data, m->size, 1, f) != 1) {
		log_warn("%s: %s: fwrite", a->name, path);
		xfree(path);
		return (DELIVER_FAILURE);
	}
	fclose(f);

	xfree(path);
	return (DELIVER_SUCCESS);
}
