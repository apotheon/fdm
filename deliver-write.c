/* $Id: deliver-write.c,v 1.24 2007-03-08 15:44:52 nicm Exp $ */

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

int	 deliver_write_deliver(struct deliver_ctx *, struct action *);
void	 deliver_write_desc(struct action *, char *, size_t);

struct deliver deliver_write = {
	DELIVER_ASUSER,
	deliver_write_deliver,
	deliver_write_desc
};

int
deliver_write_deliver(struct deliver_ctx *dctx, struct action *t)
{
	return (do_write(dctx, t, 0));
}

void
deliver_write_desc(struct action *t, char *buf, size_t len)
{
	struct deliver_write_data	*data = t->data;

	xsnprintf(buf, len, "write \"%s\"", data->path.str);
}

int
do_write(struct deliver_ctx *dctx, struct action *t, int appendf)
{
	struct account			*a = dctx->account;
	struct mail			*m = dctx->mail;
	struct deliver_write_data	*data = t->data;
        char				*path;
        FILE    			*f;

	path = replace(&data->path, m->tags, m, *dctx->pm_valid, dctx->pm);
        if (path == NULL || *path == '\0') {
		if (path != NULL)
			xfree(path);
		log_warnx("%s: empty command", a->name);
                return (DELIVER_FAILURE);
        }

	if (appendf)
		log_debug("%s: appending to %s", a->name, path);
	else
		log_debug("%s: writing to %s", a->name, path);
        f = fopen(path, appendf ? "a" : "w");
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
