/* $Id: deliver-remove-from-cache.c,v 1.1 2007-10-02 10:04:43 nicm Exp $ */

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

int	 deliver_remove_from_cache_deliver(
	     struct deliver_ctx *, struct actitem *);
void	 deliver_remove_from_cache_desc(struct actitem *, char *, size_t);

struct deliver deliver_remove_from_cache = {
	"remove-from-cache",
	DELIVER_INCHILD,
	deliver_remove_from_cache_deliver,
	deliver_remove_from_cache_desc
};

int
deliver_remove_from_cache_deliver(struct deliver_ctx *dctx, struct actitem *ti)
{
	struct account				*a = dctx->account;
	struct mail				*m = dctx->mail;
	struct deliver_remove_from_cache_data	*data = ti->data;
	char					*key;
	struct cache				*cache;

	key = replacestr(&data->key, m->tags, m, &m->rml);
	if (key == NULL || *key == '\0') {
		log_warnx("%s: empty key", a->name);
		goto error;
	}
	log_debug2("%s: removing from cache %s: %s", a->name, data->path, key);

	TAILQ_FOREACH(cache, &conf.caches, entry) {
		if (strcmp(data->path, cache->path) == 0) {
			if (open_cache(a, cache) != 0)
				goto error;
			if (db_contains(cache->db, key) &&
			    db_remove(cache->db, key) != 0) {
				log_warnx(
				    "%s: error removing from cache %s: %s",
				    a->name, cache->path, key);
				goto error;
			}
			xfree(key);
			return (DELIVER_SUCCESS);
		}
	}
	log_warnx("%s: cache %s not declared", a->name, data->path);

error:
	if (key != NULL)
		xfree(key);
	return (DELIVER_FAILURE);
}

void
deliver_remove_from_cache_desc(struct actitem *ti, char *buf, size_t len)
{
	struct deliver_remove_from_cache_data	*data = ti->data;

	xsnprintf(buf, len,
	    "remove-from-cache \"%s\" key \"%s\"", data->path, data->key.str);
}
