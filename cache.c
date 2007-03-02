/* $Id: cache.c,v 1.21 2007-03-02 18:32:12 nicm Exp $ */

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
#include <sys/stat.h>

#include <fcntl.h>
#include <fnmatch.h>
#include <stdlib.h>
#include <string.h>

#include "fdm.h"

/* With gcc 2.95.x, you can't include zlib.h before openssl.h. */
#include <zlib.h>

struct cache *cache_reference; /* XXX */

int	cache_sortcmp(const void *, const void *);
int	cache_searchcmp(const void *, const void *);

int
cache_sortcmp(const void *ptr1, const void *ptr2)
{
	const struct cacheent	*ce1 = ptr1;
	const struct cacheent	*ce2 = ptr2;
	const char		*key1, *key2;

	if (!ce1->used && ce2->used)
		return (1);
	else if (ce1->used && !ce2->used)
		return (-1);
	else if (!ce1->used && !ce2->used)
		return (0);

	key1 = CACHE_KEY(cache_reference, ce1);
	key2 = CACHE_KEY(cache_reference, ce2);
	return (strcmp(key1, key2));
}

int
cache_searchcmp(const void *ptr1, const void *ptr2)
{
	const struct cacheent	*ce = ptr2;
	const char		*key1 = ptr1;
	const char		*key2;

	if (!ce->used)
		return (-1);

	key2 = CACHE_KEY(cache_reference, ce);
	return (strcmp(key1, key2));
}

int
cache_save(struct cache **cp, const char *path)
{
	struct cache	*c = *cp;
	u_long		 checksum;
	int		 fd = -1;

	if ((fd = openlock(path, conf.lock_types, O_WRONLY|O_CREAT, 0)) != 0)
		goto error;

	checksum = adler32(0, Z_NULL, 0);
	checksum = adler32((uLong) checksum, (Bytef *) c, (uInt) CACHE_SIZE(c));
	
	if (write(fd, &checksum, sizeof checksum) != 0)
		goto error;
	if (write(fd, c, CACHE_SIZE(c)) != 0)
		goto error;

	close(fd);
	return (0);

error:
	if (fd != -1)
		close(fd);
	return (1);
}

int
cache_load(struct cache **cp, const char *path, int *bad)
{
	struct cache	*c = NULL;
	struct stat	sb;
	u_long		checksum, in;
	int 		fd = -1;
	ssize_t		n;

	*bad = 0;

	if (stat(path, &sb) != 0)
		goto error;
	if (sb.st_size < sizeof checksum)
		goto bad;

	if ((fd = openlock(path, conf.lock_types, O_RDONLY, 0)) != 0)
		goto error;

	if ((n = read(fd, &in, sizeof in)) == -1)
		goto bad;
	if (n != sizeof in)
		goto bad;
	
	c = *cp = xmalloc(sb.st_size);
	if ((n = read(fd, c, sb.st_size)) == -1)
		goto bad;
	if (n != 4)
		goto bad;

	checksum = adler32(0, NULL, 0);
	checksum = adler32((uLong) checksum, (Bytef *) c, (uInt) CACHE_SIZE(c));
	if (checksum != in)
		goto bad;
	
	close(fd);
	return (0);

bad:
	*bad = 1;

error:
	if (fd != -1)
		close(fd);
	if (c != NULL)
		xfree(c);
	return (1);
}

void
cache_create(struct cache **cp)
{
	*cp = xmalloc(sizeof **cp);
	cache_clear(cp);
}

void
cache_clear(struct cache **cp)
{
	struct cache	*c = *cp;

	c->entries = CACHEENTRIES;

	c->str_size = CACHEBUFFER;
	c->str_used = 0;

	c->sorted = 1;

	*cp = xrealloc(c, 1, CACHE_SIZE(c));
	memset(CACHE_ENTRY(*cp, 0), 0, CACHE_ENTRYSIZE(*cp));
}

void
cache_destroy(struct cache **cp)
{
	xfree(*cp);
	*cp = NULL;
}

void
cache_dump(struct cache *c, const char *prefix, void (*p)(const char *, ...))
{
	struct cacheent *ce;
	u_int		 i;

	for (i = 0; i < c->entries; i++) {
		ce = CACHE_ENTRY(c, i);
		if (!ce->used)
			continue;
		p("%s: %s: %.*s", prefix, CACHE_KEY(c, ce), 
		    ce->length, (char *) CACHE_VALUE(c, ce));
	}
}

void
cache_add(struct cache **cp, const char *key, const void *val, size_t vallen)
{
	struct cache	*c = *cp;
	size_t		 size, keylen;
	u_int		 i, entries;
	struct cacheent *ce;

	keylen = strlen(key) + 1;

	ce = CACHE_ENTRY(c, 0);
	size = c->str_size;
	while (c->str_size - c->str_used < keylen + vallen) {
		if (CACHE_SIZE(c) > SIZE_MAX / 2)
			fatalx("cache_add: size too large");
		c->str_size *= 2;
	}
	if (size != c->str_size) {
		c = *cp = xrealloc(c, 1, CACHE_SIZE(c));
		memmove(CACHE_ENTRY(c, 0), ce, CACHE_ENTRYSIZE(c));
	}

	ce = cache_find(c, key);
	if (ce == NULL) {
		/* unused entries start at the end if sorted */
		for (i = c->entries; i > 0; i--) {
			ce = CACHE_ENTRY(c, i - 1);
			if (!ce->used)
				break;
		}
		if (i == 0) {
			/* allocate some more */
			if (c->entries > UINT_MAX / 2)
				fatalx("cache_add: entries too large");
			entries = c->entries;
			
			c->entries *= 2;
			c = *cp = xrealloc(c, 1, CACHE_SIZE(c));

			memset(CACHE_ENTRY(c, entries), 0,
			    CACHE_ENTRYSIZE(c) / 2);
			ce = CACHE_ENTRY(c, c->entries - 1);
		}
		ce->key = c->str_used;
		memcpy(CACHE_KEY(c, ce), key, keylen);
		c->str_used += keylen;
	}
	ce->value = c->str_used;
	ce->length = vallen;
	memcpy(CACHE_VALUE(c, ce), val, vallen);
	c->str_used += vallen;

 	if (!ce->used) {
		/* if replacing an existing key, there is no need to resort */
		c->sorted = 0;
	}
	ce->used = 1;
}

void
cache_delete(struct cache **cp, struct cacheent *ce)
{
	(*cp)->sorted = 0;
	ce->used = 0;
}

struct cacheent *
cache_find(struct cache *c, const char *key)
{
	struct cacheent	*ce;

	cache_reference = c;

	if (!c->sorted)
		qsort(CACHE_ENTRY(c, 0), c->entries, sizeof *ce, cache_sortcmp);
	ce = bsearch(key,
	    CACHE_ENTRY(c, 0), c->entries, sizeof *ce, cache_searchcmp);
	return (ce);
}

struct cacheent *
cache_match(struct cache *c, const char *pattern)
{
	struct cacheent	*ce;
	u_int		 i;

	ce = NULL;
	for (i = 0; i < c->entries; i++) {
		ce = CACHE_ENTRY(c, i);
		if (ce->used && fnmatch(pattern, CACHE_KEY(c, ce), 0) == 0)
			break;
	}
	if (i == c->entries)
		return (NULL);
	return (ce);
}
