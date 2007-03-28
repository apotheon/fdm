/* $Id: deliver-append.c,v 1.14 2007-03-28 19:59:57 nicm Exp $ */

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

int	 deliver_append_deliver(struct deliver_ctx *, struct actitem *);
void	 deliver_append_desc(struct actitem *, char *, size_t);

struct deliver deliver_append = {
	"append",
	DELIVER_ASUSER,
	deliver_append_deliver,
	deliver_append_desc
};

int
deliver_append_deliver(struct deliver_ctx *dctx, struct actitem *ti)
{
	return (do_write(dctx, ti, 1));
}

void
deliver_append_desc(struct actitem *ti, char *buf, size_t len)
{
	struct deliver_write_data	*data = ti->data;

	xsnprintf(buf, len, "append \"%s\"", data->path.str);
}
