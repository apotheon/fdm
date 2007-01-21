/* $Id: fetch-nntp.c,v 1.28 2007-01-21 14:32:26 nicm Exp $ */

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
#include <sys/stat.h>

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "fdm.h"

int	nntp_init(struct account *);
int	nntp_free(struct account *);
int	nntp_connect(struct account *);
int	nntp_disconnect(struct account *);
int	nntp_poll(struct account *, u_int *);
int	nntp_fetch(struct account *, struct mail *);
int	nntp_delete(struct account *);
int	nntp_keep(struct account *);
char   *nntp_desc(struct account *);

int	nntp_code(char *);
char   *nntp_line(struct account *, char **, size_t *, const char *);
char   *nntp_check(struct account *, char **, size_t *, const char *, u_int *);
int	nntp_is(struct account *, char *, const char *, u_int, u_int);
int	nntp_group(struct account *, char **, size_t *);
int	nntp_save(struct account *);

struct fetch	fetch_nntp = { { "nntp", NULL },
			       nntp_init,
			       nntp_connect,
			       nntp_poll,
			       nntp_fetch,
			       NULL,
			       nntp_delete,
			       nntp_keep,
			       nntp_disconnect,
			       nntp_free,
			       nntp_desc
};

int
nntp_code(char *line)
{
	char		 ch;
	const char	*errstr;
	int	 	 n;
	size_t		 len;

	len = strspn(line, "0123456789");
	if (len == 0)
		return (-1);
	ch = line[len];
	line[len] = '\0';

	n = strtonum(line, 100, 999, &errstr);
	line[len] = ch;
	if (errstr != NULL)
		return (-1);

	return (n);
}

char *
nntp_line(struct account *a, char **lbuf, size_t *llen, const char *s)
{
	struct nntp_data	*data = a->data;
	char			*line, *cause;

	switch (io_pollline2(data->io, &line, lbuf, llen, &cause)) {
	case 0:
		log_warnx("%s: %s: connection unexpectedly closed", a->name, s);
		return (NULL);
	case -1:
		log_warnx("%s: %s: %s", a->name, s, cause);
		xfree(cause);
		return (NULL);
	}

	return (line);
}

char *
nntp_check(struct account *a, char **lbuf, size_t *llen, const char *s,
    u_int *code)
{
	char	*line;

restart:
	if ((line = nntp_line(a, lbuf, llen, s)) == NULL)
		return (NULL);

	*code = nntp_code(line);
	if (*code >= 100 && *code <= 199)
		goto restart;

	return (line);
}

int
nntp_is(struct account *a, char *line, const char *s, u_int code, u_int n)
{
	if (code != n) {
		log_warnx("%s: %s: unexpected data: %s", a->name, s, line);
		return (0);
	}
	return (1);
}

int
nntp_group(struct account *a, char **lbuf, size_t *llen)
{
	struct nntp_data	*data = a->data;
	struct nntp_group	*group;
	char			*line, *ptr, *ptr2;
	size_t			 len;
	u_int			 code;

	group = ARRAY_ITEM(data->groups, data->group, struct nntp_group *);
	io_writeline(data->io, "GROUP %s", group->name);

	if ((line = nntp_check(a, lbuf, llen, "GROUP", &code)) == NULL)
		return (1);
	if (!nntp_is(a, line, "GROUP", code, 211))
		return (1);
	if (sscanf(line, "211 %u %*u %u", &group->size, &group->last) != 2) {
 		log_warnx("%s: GROUP: invalid response: %s", a->name, line);
		return (1);
	}
	if (cache_get(data->cache, group->name, &group->first) == 0) {
		log_debug("%s: last message number is %u, was %u", a->name, 
		    group->last, group->first);
		group->size = group->last - group->first;
		io_writeline(data->io, "STAT %u", group->first);
		if ((line = nntp_check(a, lbuf, llen, "STAT", &code)) == NULL)
			return (1);
		if (!nntp_is(a, line, "STAT", code, 223))
			return (1);
		
		ptr = strchr(line, '<');
		ptr2 = NULL;
		if (ptr != NULL)
			ptr2 = strchr(ptr, '>');
		if (ptr != NULL && ptr2 != NULL) {
			ptr++;
			len = ptr2 - ptr;
			group->id = xmalloc(len + 1);
			memcpy(group->id, ptr, len);
			group->id[len] = '\0';
		}
	} else
		log_debug("%s: last message number is %u", a->name, group->last);
	cache_put(data->cache, group->name, group->last); /* XXX */
	/* 
	   XXX as each mail is deleted/kept, set last to its message number
	   XXX check message id of current last when GROUP
	   XXX lose the caching stuff?
	 */

	return (0);
}

int
nntp_save(struct account *a)
{
	struct nntp_data	*data = a->data;
	struct nntp_group	*group;
	FILE			*f;
	u_int			 i;
	off_t			 off;

	if (lseek(data->fd, 0, SEEK_SET) == -1) {
		log_warn("%s: lseek", a->name);
		return (1);
	}

	if ((f = fdopen(data->fd, "r+")) == NULL) {
		log_warn("%s: fdopen", a->name);
		return (1);
	}
	
	for (i = 0; i < ARRAY_LENGTH(data->groups); i++) {
		group = ARRAY_ITEM(data->groups, i, struct nntp_group *);
		fprintf(f, "%zu %s %u %zu %s", strlen(group->name), group->name,
		    group->last, strlen(group->id), group->id);
	}

	if ((off = lseek(data->fd, 0, SEEK_CUR)) == -1) {
		log_warn("%s: lseek", a->name);
		return (1);
	}
	if (ftruncate(data->fd, off) == -1) {
		log_warn("%s: ftruncate", a->name);
		return (1);
	}

	fclose(f);
	return (0);
}

int
nntp_init(struct account *a)
{
	struct nntp_data	*data = a->data;
	char			*cause;
	char			*path;

	data->cache = cache_open(data->path, &cause);
	if (data->cache == NULL) {
		log_warnx("%s: %s", a->name, cause);
		xfree(cause);
		return (1);
	}

	/* XXX lock will be unremovable as root */
	xasprintf(&path, "%s.groups", data->path);
	data->fd = openlock(path, conf.lock_types, O_CREAT|O_WRONLY|O_APPEND,
	    S_IRUSR|S_IWUSR);
	if (data->fd == -1) {
		log_warn("%s: %s", a->name, path);
		xfree(path);
		return (1);
	}
	xfree(path);

	data->group = 0;

	return (0);
}

int
nntp_free(struct account *a)
{
	struct nntp_data	*data = a->data;
	char			*path;

	if (data->key != NULL)
		xfree(data->key);

	nntp_save(a);

	xasprintf(&path, "%s.groups", data->path);
	closelock(data->fd, path, conf.lock_types);
	xfree(path);

	return (0);
}

int
nntp_connect(struct account *a)
{
	struct nntp_data	*data = a->data;
	char			*lbuf, *line, *cause;
	size_t			 llen;
	u_int			 n, total, code;

	data->io = connectproxy(&data->server, conf.proxy, IO_CRLF, &cause);
	if (data->io == NULL) {
		cache_close(data->cache);
		log_warnx("%s: %s", a->name, cause);
		xfree(cause);
		return (1);
	}
	if (conf.debug > 3 && !conf.syslog)
		data->io->dup_fd = STDOUT_FILENO;

	n = cache_compact(data->cache, data->expiry, &total);
	log_debug("%s: cache has %u entries", a->name, total);
	log_debug("%s: expired %u entries", a->name, n);

	llen = IO_LINESIZE;
	lbuf = xmalloc(llen);

	if ((line = nntp_check(a, &lbuf, &llen, "CONNECT", &code)) == NULL)
		goto error;
	if (!nntp_is(a, line, "CONNECT", code, 200))
		goto error;

	if (nntp_group(a, &lbuf, &llen) != 0)
		goto error;

	xfree(lbuf);
	return (0);

error:
	cache_close(data->cache);

	io_writeline(data->io, "QUIT");
	io_flush(data->io, NULL);

	io_close(data->io);
	io_free(data->io);

	xfree(lbuf);
	return (1);
}

int
nntp_disconnect(struct account *a)
{
	struct nntp_data	*data = a->data;
	char			*lbuf, *line;
	size_t			 llen;
	u_int			 code;

	cache_close(data->cache);

	llen = IO_LINESIZE;
	lbuf = xmalloc(llen);

	io_writeline(data->io, "QUIT");
	if ((line = nntp_check(a, &lbuf, &llen, "QUIT", &code)) == NULL)
		goto error;
	if (!nntp_is(a, line, "QUIT", code, 205))
		goto error;

	io_close(data->io);
	io_free(data->io);

	xfree(lbuf);
	return (0);

error:
	io_writeline(data->io, "QUIT");
	io_flush(data->io, NULL);

	io_close(data->io);
	io_free(data->io);

	xfree(lbuf);
	return (1);
}

int
nntp_poll(struct account *a, u_int *n)
{
	struct nntp_data       	*data = a->data;
	struct nntp_group	*group;
	char			*lbuf;
	size_t			 llen;

	llen = IO_LINESIZE;
	lbuf = xmalloc(llen);

	*n = 0;
	for (;;) {
		group = ARRAY_ITEM(data->groups, data->group,
		    struct nntp_group *);
		(*n) += group->size;

		data->group++;
		if (data->group == ARRAY_LENGTH(data->groups))
			break;
		if (nntp_group(a, &lbuf, &llen) != 0)
			goto error;
	}

	xfree(lbuf);
	return (0);

error:
	xfree(lbuf);
	return (1);
}

int
nntp_fetch(struct account *a, struct mail *m)
{
	struct nntp_data	*data = a->data;
	char			*lbuf, *line, *ptr, *ptr2;
	size_t			 llen, off, len;
	u_int			 lines, code;
	int			 flushing;

	llen = IO_LINESIZE;
	lbuf = xmalloc(llen);

restart:
	io_writeline(data->io, "NEXT");
	if ((line = nntp_check(a, &lbuf, &llen, "NEXT", &code)) == NULL)
		goto error;
	if (code == 421) {
		data->group++;
		if (data->group == ARRAY_LENGTH(data->groups)) {
			xfree(lbuf);
			return (FETCH_COMPLETE);
		}
		if (nntp_group(a, &lbuf, &llen) != 0)
			goto error;
		goto restart;
	}
	if (!nntp_is(a, line, "NEXT", code, 223)) {
		log_warnx("%s: NEXT: unexpected response: %s", a->name, line);
		goto restart;
	}

	/* find message-id */
	ptr = strchr(line, '<');
	ptr2 = NULL;
	if (ptr != NULL)
		ptr2 = strchr(ptr, '>');
	if (ptr == NULL || ptr2 == NULL) {
		log_warnx("%s: NEXT: malformed response: %s", a->name, line);
		goto restart;
	}

	ptr++;
	len = ptr2 - ptr;
	data->key = xmalloc(len + 1);
	memcpy(data->key, ptr, len);
	data->key[len] = '\0';

	if (cache_contains(data->cache, data->key)) {
		log_debug3("%s: found in cache: %s", a->name, data->key);
		cache_update(data->cache, data->key);

		xfree(data->key);
		data->key = NULL;
		goto restart;
	}
	log_debug2("%s: new: %s", a->name, data->key);

	/* retrieve the article */
	io_writeline(data->io, "ARTICLE");
	if ((line = nntp_check(a, &lbuf, &llen, "ARTICLE", &code)) == NULL)
		goto error;
	if (code == 423 || code == 430) {
		xfree(data->key);
		data->key = NULL;
		goto restart;
	}
	if (!nntp_is(a, line, "ARTICLE", code, 220))
		goto error;

	mail_open(m, IO_BLOCKSIZE);
	m->s = xstrdup(ARRAY_ITEM(data->groups, data->group, char *));

	flushing = 0;
	off = lines = 0;
	for (;;) {
		if ((line = nntp_line(a, &lbuf, &llen, "ARTICLE")) == NULL)
			goto error;

		if (line[0] == '.' && line[1] == '.')
			line++;
		else if (line[0] == '.') {
			m->size = off;
			break;
		}

		len = strlen(line);
		if (len == 0 && m->body == -1)
			m->body = off + 1;

		if (!flushing) {
			resize_mail(m, off + len + 1);

			if (len > 0)
				memcpy(m->data + off, line, len);
			m->data[off + len] = '\n';
		}

		lines++;
		off += len + 1;
		if (off + lines > conf.max_size)
			flushing = 1;
	}

	xfree(lbuf);
	if (flushing)
		return (FETCH_OVERSIZE);
	return (FETCH_SUCCESS);

error:
	xfree(lbuf);
	return (FETCH_ERROR);
}

int
nntp_delete(struct account *a)
{
	struct nntp_data	*data = a->data;

	cache_add(data->cache, data->key);

	xfree(data->key);
	data->key = NULL;

	return (0);
}

int
nntp_keep(struct account *a)
{
	struct nntp_data	*data = a->data;

	xfree(data->key);
	data->key = NULL;

	return (0);
}

char *
nntp_desc(struct account *a)
{
	struct nntp_data	*data = a->data;
	char			*s, *groups;

	groups = xstrdup("XXX"); //fmt_strings("groups ", data->groups); /*XXX*/
	xasprintf(&s, "nntp server \"%s\" port %s %s cache \"%s\" expiry %lld "
	    "seconds", data->server.host, data->server.port, groups,
	    data->path, data->expiry);
	xfree(groups);
	return (s);
}
