/* $Id: privsep.c,v 1.13 2011-09-14 13:36:19 nicm Exp $ */

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

#include "fdm.h"

int
privsep_send(struct io *io, struct msg *msg, struct msgbuf *msgbuf)
{
	char *cause;

	msg->size = 0;
	if (msgbuf != NULL && msgbuf->buf != NULL && msgbuf->len > 0)
		msg->size = msgbuf->len;

	io_write(io, msg, sizeof *msg);
	if (io_flush(io, INFTIM, &cause) != 0)
		return (-1);

	if (msg->size != 0) {
		io_write(io, msgbuf->buf, msgbuf->len);
		if (io_flush(io, INFTIM, &cause) != 0)
			return (-1);
	}

	return (0);
}

int
privsep_check(struct io *io)
{
	return (IO_RDSIZE(io) >= sizeof (struct msg));
}

int
privsep_recv(struct io *io, struct msg *msg, struct msgbuf *msgbuf)
{
	char	*tmpbuf;

	if (msgbuf != NULL) {
		msgbuf->buf = NULL;
		msgbuf->len = 0;
	}

	if (io_wait(io, sizeof *msg, INFTIM, NULL) != 0)
		return (-1);
	if (io_read2(io, msg, sizeof *msg) != 0)
		return (-1);

	if (msg->size == 0)
		return (0);

	if (io_wait(io, msg->size, INFTIM, NULL) != 0)
		return (-1);
	if (msgbuf == NULL) {
		if ((tmpbuf = io_read(io, msg->size)) == NULL)
			return (-1);
		xfree(tmpbuf);
	} else {
		if ((msgbuf->buf = io_read(io, msg->size)) == NULL)
			return (-1);
		msgbuf->len = msg->size;
	}

	return (0);
}
