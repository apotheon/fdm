/* $Id: fetch-stdin.c,v 1.58 2007-05-18 16:19:34 nicm Exp $ */

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
#include "fetch.h"

int	 fetch_stdin_connect(struct account *);
int	 fetch_stdin_completed(struct account *);
int	 fetch_stdin_fetch(struct account *, struct fetch_ctx *fctx);
int	 fetch_stdin_disconnect(struct account *);
void	 fetch_stdin_desc(struct account *, char *, size_t);

struct fetch fetch_stdin = {
	"stdin",
	fetch_stdin_connect,
	NULL,
	NULL,
	fetch_stdin_completed,
	NULL,
	fetch_stdin_fetch,
	NULL,
	NULL,
	NULL,
	fetch_stdin_disconnect,
	fetch_stdin_desc
};

int
fetch_stdin_connect(struct account *a)
{
	struct fetch_stdin_data	*data = a->data;

	data->llen = IO_LINESIZE;
	data->lbuf = xmalloc(data->llen);

	if (isatty(STDIN_FILENO)) {
		log_warnx("%s: stdin is a tty. ignoring", a->name);
		return (-1);
	}

	if (fcntl(STDIN_FILENO, F_GETFL) == -1) {
		if (errno != EBADF)
			fatal("fcntl");
		log_warnx("%s: stdin is invalid", a->name);
		return (-1);
	}

	data->io = io_create(STDIN_FILENO, NULL, IO_LF, conf.timeout);
	if (conf.debug > 3 && !conf.syslog)
		data->io->dup_fd = STDOUT_FILENO;

	data->lines = 0;
	data->bodylines = -1;

	data->complete = 0;

	return (0);
}

int
fetch_stdin_completed(struct account *a)
{
	struct fetch_stdin_data	*data = a->data;

	return (data->complete);
}

int
fetch_stdin_disconnect(struct account *a)
{
	struct fetch_stdin_data	*data = a->data;

	if (data->io != NULL)
		io_free(data->io);

	close(STDIN_FILENO);

	xfree(data->lbuf);

	return (0);
}

int
fetch_stdin_fetch(struct account *a, struct fetch_ctx *fctx)
{
	struct fetch_stdin_data	*data = a->data;
	struct mail		*m;
	int		 	 error;
	char			*line, *cause;
	size_t			 len;

	/* Flush deleted once complete. */  
	if (data->complete) {
		while (done_mail(a, fctx) != NULL)
			dequeue_mail(a, fctx);
		return (FETCH_HOLD);
	}

	/* Initialise the mail. */
	m = xcalloc(1, sizeof *m);
	if (mail_open(m, IO_BLOCKSIZE) != 0) {
		log_warn("%s: failed to create mail", a->name);
		return (FETCH_ERROR);
	}
	m->size = 0;

	m->auxdata = NULL;
	m->auxfree = NULL;
	
	/* Add default tags. */
	default_tags(&m->tags, NULL, a);

	for (;;) {
		/* 
		 * There can only be one mail on stdin so reentrancy is
		 * irrelevent. This is a good thing since we want to check for
		 * close which means end of mail.
		 */
		error = io_pollline2(data->io, 
		    &line, &data->lbuf, &data->llen, &cause);
		if (error == 0) {
			/* Normal close is fine. */
			break;
		} else if (error == -1) {
			if (errno == EAGAIN)
				continue;
			log_warnx("%s: %s", a->name, cause);
			xfree(cause);
			return (FETCH_ERROR);
		}

		len = strlen(line);
		if (len == 0 && m->body == -1) {
			m->body = m->size + 1;
			data->bodylines = 0;
		}
		data->lines++;
		if (data->bodylines != -1)
			data->bodylines++;
		
		if (mail_resize(m, m->size + len + 1) != 0) {
			log_warn("%s: failed to resize mail", a->name);
			return (FETCH_ERROR);
		}
		if (len > 0)
			memcpy(m->data + m->size, line, len);
		
		/* Append an LF. */
		m->data[m->size + len] = '\n';
		m->size += len + 1;
		
		if (m->size > conf.max_size) {
			oversize_mail(a, fctx, m);
			data->complete = 1;
			return (FETCH_HOLD);
		}
	}

	/* Tag mail. */
	add_tag(&m->tags, "lines", "%u", data->lines);
	if (data->bodylines == -1) {
		add_tag(&m->tags, "body_lines", "0");
		add_tag(&m->tags, "header_lines", "%u", data->lines - 1);
	} else {
		add_tag(&m->tags, "body_lines", "%d", data->bodylines - 1);
		add_tag(&m->tags, "header_lines", "%d", data->lines -
		    data->bodylines);
	}

	transform_mail(a, fctx, m);
	if (m->size == 0) {
		if (empty_mail(a, fctx, m) != 0)
			return (FETCH_ERROR);
		data->complete = 1;
		return (FETCH_HOLD);
	}
	enqueue_mail(a, fctx, m);

	data->complete = 1;
	return (FETCH_HOLD);
}

void
fetch_stdin_desc(unused struct account *a, char *buf, size_t len)
{
	strlcpy(buf, "stdin", len);
}
