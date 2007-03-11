/* $Id: fetch-imap.c,v 1.57 2007-03-11 19:04:03 nicm Exp $ */

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

#include <unistd.h>

#include "fdm.h"
#include "fetch.h"

int	 	 fetch_imap_connect(struct account *);
int	 	 fetch_imap_disconnect(struct account *);
int	 	 fetch_imap_free(struct account *);
void		 fetch_imap_desc(struct account *, char *, size_t);

int printflike2	 fetch_imap_putln(struct account *, const char *, ...);
char	        *fetch_imap_getln(struct account *, int);
void		 fetch_imap_flush(struct account *);

struct fetch fetch_imap = {
	{ "imap", "imaps" },
	imap_init,	/* from imap-common.c */
	fetch_imap_connect,
	imap_poll,	/* from imap-common.c */
	imap_fetch,	/* from imap-common.c */
	imap_purge,	/* from imap-common.c */
	imap_delete,	/* from imap-common.c */
	imap_keep,	/* from imap-common.c */
	fetch_imap_disconnect,
	fetch_imap_free,
	fetch_imap_desc
};

int printflike2
fetch_imap_putln(struct account *a, const char *fmt, ...)
{
	struct fetch_imap_data	*data = a->data;

	va_list	ap;

	va_start(ap, fmt);
	io_vwriteline(data->io, fmt, ap);
	va_end(ap);

	return (0);
}

char *
fetch_imap_getln(struct account *a, int type)
{
	struct fetch_imap_data	*data = a->data;
	char		       **lbuf = &data->lbuf;
	size_t			*llen = &data->llen;
	char			*line, *cause;
	int			 tag;

restart:
	switch (io_pollline2(data->io, &line, lbuf, llen, &cause)) {
	case 0:
		log_warnx("%s: connection unexpectedly closed", a->name);
		return (NULL);
	case -1:
		log_warnx("%s: %s", a->name, cause);
		xfree(cause);
		return (NULL);
	}

	if (type == IMAP_RAW)
		return (line);
	tag = imap_tag(line);
	switch (type) {
	case IMAP_TAGGED:
		if (tag == IMAP_TAG_NONE)
			goto restart;
		if (tag == IMAP_TAG_CONTINUE)
			goto invalid;
		if (tag != data->tag)
			goto invalid;
		break;
	case IMAP_UNTAGGED:
		if (tag != IMAP_TAG_NONE)
			goto invalid;
		break;
	case IMAP_CONTINUE:
		if (tag == IMAP_TAG_NONE)
			goto restart;
		if (tag != IMAP_TAG_CONTINUE)
			goto invalid;
		break;
	}

	return (line);

invalid:
	log_warnx("%s: unexpected data: %s", a->name, line);
	return (NULL);
}

void
fetch_imap_flush(struct account *a)
{
	struct fetch_imap_data	*data = a->data;

	io_flush(data->io, NULL);
}

int
fetch_imap_connect(struct account *a)
{
	struct fetch_imap_data	*data = a->data;
	char			*cause;

	data->io = connectproxy(&data->server, conf.proxy, IO_CRLF,
	    conf.timeout * 1000, &cause);
	if (data->io == NULL) {
		log_warnx("%s: %s", a->name, cause);
		xfree(cause);
		return (1);
	}
	if (conf.debug > 3 && !conf.syslog)
		data->io->dup_fd = STDOUT_FILENO;

	data->getln = fetch_imap_getln;
	data->putln = fetch_imap_putln;
	data->flush = fetch_imap_flush;
	data->src = data->server.host;

	if (imap_login(a) != 0)
		return (1);

	if (imap_select(a) != 0) {
		imap_abort(a);
		return (1);
	}

	return (0);
}

int
fetch_imap_disconnect(struct account *a)
{
	struct fetch_imap_data	*data = a->data;

	if (imap_close(a) != 0)
		goto error;
	if (imap_logout(a) != 0)
		goto error;

	return (0);

error:
	imap_abort(a);

	return (1);
}

int
fetch_imap_free(struct account *a)
{
	struct fetch_imap_data	*data = a->data;

	if (data->io != NULL) { 
		io_close(data->io);
		io_free(data->io);
	}

	if (imap_free(a) != 0)
		return (1);
		
	return (0);
}

void
fetch_imap_desc(struct account *a, char *buf, size_t len)
{
	struct fetch_imap_data	*data = a->data;

	xsnprintf(buf, len,
	    "imap%s server \"%s\" port %s user \"%s\" folder \"%s\"",
	    data->server.ssl ? "s" : "", data->server.host, data->server.port,
	    data->user, data->folder);
}
