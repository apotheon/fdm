/* $Id: fetch-stdin.c,v 1.38 2007-02-09 16:48:10 nicm Exp $ */

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
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "fdm.h"

int	 stdin_connect(struct account *);
int	 stdin_disconnect(struct account *);
int	 stdin_fetch(struct account *, struct mail *);
int	 stdin_delete(struct account *);
void	 stdin_desc(struct account *, char *, size_t);

struct fetch	fetch_stdin = { { NULL, NULL },
				NULL,
				stdin_connect,
				NULL,
				stdin_fetch,
				NULL,
				stdin_delete,
				NULL,
				stdin_disconnect,
				NULL,
				stdin_desc
};

int
stdin_connect(struct account *a)
{
	struct stdin_data	*data = a->data;

	if (isatty(STDIN_FILENO)) {
		log_warnx("%s: stdin is a tty. ignoring", a->name);
		return (1);
	}

	if (fcntl(STDIN_FILENO, F_GETFL) == -1) {
		if (errno != EBADF)
			fatal("fcntl");
		log_warnx("%s: stdin is invalid", a->name);
		return (1);
	}

	data->io = io_create(STDIN_FILENO, NULL, IO_LF, conf.timeout * 1000);
	if (conf.debug > 3 && !conf.syslog)
		data->io->dup_fd = STDOUT_FILENO;

	data->complete = 0;

	return (0);
}

int
stdin_disconnect(struct account *a)
{
	struct stdin_data	*data = a->data;

	io_free(data->io);

	close(STDIN_FILENO);

	return (0);
}

int
stdin_delete(struct account *a)
{
	struct stdin_data	*data = a->data;
	char		        *line, *lbuf;
	size_t			 llen;

	llen = IO_LINESIZE;
	lbuf = xmalloc(llen);

	while (io_pollline2(data->io, &line, &lbuf, &llen, NULL) == 1)
		;

	xfree(lbuf);
	return (0);
}

int
stdin_fetch(struct account *a, struct mail *m)
{
	struct stdin_data	*data = a->data;
	int		 	 error;
	char			*line, *cause, *lbuf;
	size_t			 len, llen;

	if (data->complete)
		return (FETCH_COMPLETE);

	if (m->data == NULL) {
		mail_open(m, IO_BLOCKSIZE);
		m->size = 0;
	}

	llen = IO_LINESIZE;
	lbuf = xmalloc(llen);

	for (;;) {
		error = io_pollline2(data->io, &line, &lbuf, &llen, &cause);
		if (error != 1) {
			/* normal close (error == 0) is fine */
			if (error == 0)
				break;
			log_warnx("%s: %s", a->name, cause);
			xfree(cause);
			xfree(lbuf);
			return (FETCH_ERROR);
		}

		len = strlen(line);
		if (len == 0 && m->body == -1)
			m->body = m->size + 1;

		resize_mail(m, m->size + len + 1);

		if (len > 0)
			memcpy(m->data + m->size, line, len);

		/* append an LF */
		m->data[m->size + len] = '\n';
		m->size += len + 1;

		if (m->size > conf.max_size) {
			data->complete = 1;
			xfree(lbuf);
			return (FETCH_OVERSIZE);
		}
	}

	if (m->size == 0) {
		log_warnx("%s: zero-length message", a->name);
		xfree(lbuf);
		return (FETCH_ERROR);
	}

 	data->complete = 1;
	xfree(lbuf);
	return (FETCH_SUCCESS);
}

void
stdin_desc(unused struct account *a, char *buf, size_t len)
{
	strlcpy(buf, "stdin", len);
}
