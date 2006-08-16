/* $Id: deliver-write.c,v 1.1 2006-08-16 18:58:21 nicm Exp $ */

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

int	write_deliver(struct account *, struct action *, struct mail *);

struct deliver deliver_write = { "write", write_deliver };

int
write_deliver(struct account *a, struct action *t, struct mail *m)
{
	return (do_write(a, t, m, 0));
}

int
do_write(struct account *a, struct action *t, struct mail *m, int append)
{
        char	*cmd, *map[REPL_LEN];
        FILE    *f;

	bzero(map, sizeof map);
	map[REPL_IDX('a')] = a->name;
	map[REPL_IDX('h')] = conf.home;
	map[REPL_IDX('t')] = t->name;
	map[REPL_IDX('u')] = conf.user;
	cmd = replace(t->data, map);
        if (cmd == NULL || *cmd == '\0') {
		if (cmd != NULL)
			xfree(cmd);
		log_warnx("%s: empty command", a->name);
                return (1);
        }

	if (append)
		log_debug("%s: appending to %s", a->name, cmd); 
	else
		log_debug("%s: writing to %s", a->name, cmd); 
        f = fopen(cmd, append ? "a" : "w");
        if (f == NULL) {
		log_warn("%s: %s: fopen", a->name, cmd);
		xfree(cmd);
		return (1);
	}
	if (fwrite(m->data, m->size, 1, f) != 1) {
		log_warn("%s: %s: fwrite", a->name, cmd);
		xfree(cmd);
		return (1);
	}
	fclose(f);

	xfree(cmd);	
	return (0);
}
