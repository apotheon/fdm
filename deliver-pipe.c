/* $Id: deliver-pipe.c,v 1.13 2006-11-23 17:45:32 nicm Exp $ */

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

int	 pipe_deliver(struct deliver_ctx *, struct action *);
char	*pipe_desc(struct action *);

struct deliver deliver_pipe = { DELIVER_ASUSER, pipe_deliver, pipe_desc };

int
pipe_deliver(struct deliver_ctx *dctx, struct action *t)
{
	struct account	*a = dctx->account;
	struct mail	*m = dctx->mail;
        char		*cmd;
        FILE    	*f;
	int	 	error;

	cmd = replaceinfo(t->data, a, t);
        if (cmd == NULL || *cmd == '\0') {
		log_warnx("%s: empty command", a->name);
		if (cmd != NULL)
			xfree(cmd);
                return (DELIVER_FAILURE);
        }

	log_debug("%s: piping to %s", a->name, cmd);
        f = popen(cmd, "w");
        if (f == NULL) {
		log_warn("%s: %s: popen", a->name, cmd);
		xfree(cmd);
		return (DELIVER_FAILURE);
	}
	if (fwrite(m->data, m->size, 1, f) != 1) {
		log_warn("%s: %s: fwrite", a->name, cmd);
		xfree(cmd);
		return (DELIVER_FAILURE);
	}
	if ((error = pclose(f)) != 0) {
		log_warn("%s: %s: pipe error, return value %d", a->name,
		    cmd, error);
		xfree(cmd);
		return (DELIVER_FAILURE);
	}

	xfree(cmd);
	return (DELIVER_SUCCESS);
}

char *
pipe_desc(struct action *t)
{
	char	*s;

	xasprintf(&s, "pipe \"%s\"", (char *) t->data);
	return (s);
}
