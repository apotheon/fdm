/* $Id: deliver-add-header.c,v 1.6 2007-03-12 11:21:42 nicm Exp $ */

/*
 * Copyright (c) 2007 Nicholas Marriott <nicm@users.sourceforge.net>
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

int	 deliver_add_header_deliver(struct deliver_ctx *, struct action *);
void	 deliver_add_header_desc(struct action *, char *, size_t);

struct deliver deliver_add_header = {
	DELIVER_INCHILD,
	deliver_add_header_deliver,
	deliver_add_header_desc
};

int
deliver_add_header_deliver(struct deliver_ctx *dctx, struct action *t)
{
	struct account			*a = dctx->account;
	struct mail			*m = dctx->mail;
	struct deliver_add_header_data	*data = t->data;
	char				*hdr, *value;

	hdr = replacestr(&data->hdr, m->tags, m, *dctx->pm_valid, dctx->pm);
	if (hdr == NULL || *hdr == '\0') {
		if (hdr != NULL)
			xfree(hdr);
		log_warnx("%s: empty header", a->name);
		return (DELIVER_FAILURE);
	}
	value = replacestr(&data->value, m->tags, m, *dctx->pm_valid, dctx->pm);
	if (value == NULL) {
		log_warnx("%s: bad value for header %s", a->name, hdr);
		xfree(hdr);
		return (DELIVER_FAILURE);
	}
	
	if (insert_header(m, NULL, "%s: %s", hdr, value) != 0) {
		log_warnx("%s: failed to add header %s (%s)", a->name,
		    hdr, value);
		xfree(hdr);
		xfree(value);
		return (DELIVER_FAILURE);
	}

	/* XXX needed? */
	ARRAY_FREE(&m->wrapped);
	fill_wrapped(m);

	/* invalidate the pmatch data since stuff may have moved */
	*dctx->pm_valid = 0;

	xfree(hdr);	
	xfree(value);
	return (DELIVER_SUCCESS);
}

void
deliver_add_header_desc(struct action *t, char *buf, size_t len)
{
	struct deliver_add_header_data	*data = t->data;

	xsnprintf(buf, len, 
	    "add-header \"%s\" \"%s\"", data->hdr.str, data->value.str);
}
