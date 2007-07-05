/* $Id: deliver-maildir.c,v 1.51 2007-07-05 10:00:45 nicm Exp $ */

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

#include <sys/param.h>
#include <sys/stat.h>

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "fdm.h"
#include "deliver.h"

int	 deliver_maildir_deliver(struct deliver_ctx *, struct actitem *);
void	 deliver_maildir_desc(struct actitem *, char *, size_t);

struct deliver deliver_maildir = {
	"maildir",
	DELIVER_ASUSER,
	deliver_maildir_deliver,
	deliver_maildir_desc
};

int
deliver_maildir_deliver(struct deliver_ctx *dctx, struct actitem *ti)
{
	struct account			*a = dctx->account;
	struct mail			*m = dctx->mail;
	struct deliver_maildir_data	*data = ti->data;
	static u_int			 delivered = 0;
	char				*path, ch;
	char			 	 host1[MAXHOSTNAMELEN];
	char				 host2[MAXHOSTNAMELEN], *host;
	char	 			 name[MAXPATHLEN];
	char				 src[MAXPATHLEN], dst[MAXPATHLEN];
	int	 			 exists, xfd = -1, fd = -1, len;
	ssize_t			 	 n;
	size_t	 			 first, last;
	gid_t				 gid;

	path = replacepath(&data->path, m->tags, m, &m->rml);
	if (path == NULL || *path == '\0') {
		log_warnx("%s: empty path", a->name);
		goto error;
	}
	log_debug2("%s: saving to maildir %s", a->name, path);

	/* Create the maildir directories. */
	gid = conf.file_group;
	if (checkperms(a->name, path, &exists) != 0) {
		log_warn("%s: %s: checkperms", a->name, path);
		goto error;
	}
	if (!exists && mkdir(path, DIRMODE) != 0) {
		log_warn("%s: %s: mkdir", a->name, path);
		goto error;
	} else if (!exists && gid != NOGRP && chown(path, -1, gid) == -1) {
		log_warn("%s: %s: chown", a->name, path);
		goto error;
	}
	if (printpath(name, sizeof name, "%s/cur", path) != 0) {
		log_warn("%s: %s: printpath", a->name, path);
		goto error;
	}
	if (checkperms(a->name, name, &exists) != 0) {
		log_warn("%s: %s: checkperms", a->name, name);
		goto error;
	}
	if (!exists && mkdir(name, DIRMODE) != 0) {
		log_warn("%s: %s: mkdir", a->name, name);
		goto error;
	} else if (!exists && gid != NOGRP && chown(name, -1, gid) == -1) {
		log_warn("%s: %s: chown", a->name, name);
		goto error;
	}
	if (printpath(name, sizeof name, "%s/new", path) != 0) {
		log_warn("%s: %s: printpath", a->name, path);
		goto error;
	}
	if (checkperms(a->name, name, &exists) != 0) {
		log_warn("%s: %s: checkperms", a->name, name);
		goto error;
	}
	if (!exists && mkdir(name, DIRMODE) != 0) {
		log_warn("%s: %s: mkdir", a->name, name);
		goto error;
	} else if (!exists && gid != NOGRP && chown(name, -1, gid) == -1) {
		log_warn("%s: %s: chown", a->name, name);
		goto error;
	}
	if (printpath(name, sizeof name, "%s/tmp", path) != 0) {
		log_warn("%s: %s: printpath", a->name, path);
		goto error;
	}
	if (checkperms(a->name, name, &exists) != 0) {
		log_warn("%s: %s: checkperms", a->name, name);
		goto error;
	}
	if (!exists && mkdir(name, DIRMODE) != 0) {
		log_warn("%s: %s: mkdir", a->name, name);
		goto error;
	} else if (!exists && gid != NOGRP && chown(name, -1, gid) == -1) {
		log_warn("%s: %s: chown", a->name, name);
		goto error;
	}

	if (gethostname(host1, sizeof host1) != 0)
		log_fatal("gethostname");

	/*
	 * Replace '/' with "\057" and ':' with "\072". this is a bit
	 * inefficient but sod it.
	 */
	last = strcspn(host1, "/:");
	if (host1[last] == '\0')
		host = host1;
	else {
		*host2 = '\0';

		first = 0;
		do {
			ch = host1[first + last];
			host1[first + last] = '\0';
			strlcat(host2, host1 + first, sizeof host2);
			switch (ch) {
			case '/':
				strlcat(host2, "\\057", sizeof host2);
				break;
			case ':':
				strlcat(host2, "\\072", sizeof host2);
				break;
			}
			host1[first + last] = ch;

			first += last + 1;
			last = strcspn(host1 + first, "/:");
		} while (ch != '\0');

		host = host2;
	}

restart:
	/* Find a suitable name in tmp. */
	do {
		len = xsnprintf(name, sizeof name, "%ld.%ld_%u.%s",
		    (long) time(NULL), (long) getpid(), delivered, host);
		if ((size_t) len >= sizeof name) {
			log_warn("%s: %s: xsnprintf", a->name, path);
			goto error;
		}

		if (printpath(src, sizeof src, "%s/tmp/%s", path, name) != 0) {
			log_warn("%s: %s: printpath", a->name, path);
			goto error;
		}

		fd = open(src, O_WRONLY|O_CREAT|O_EXCL, FILEMODE);
		if (fd == -1 && errno != EEXIST) {
			log_warn("%s: %s: open", a->name, src);
			goto error;
		}

		delivered++;
	} while (fd == -1);
	cleanup_register(src);

	xfd = fd;
	if (conf.file_group != NOGRP) {
		if (fchown(fd, (uid_t) -1, conf.file_group) == -1) {
			log_warn("%s: %s: fchown", a->name, path);
			goto error;
		}
	}

	/* Write the message. */
	log_debug2("%s: writing to %s", a->name, src);
	n = write(fd, m->data, m->size);
	if (n < 0 || (size_t) n != m->size || fsync(fd) != 0) {
		log_warn("%s: write", a->name);
		goto error;
	}
	close(fd);
	fd = -1;

	/*
	 * Create the new path and attempt to link it. a failed link jumps
	 * back to find another name in the tmp directory.
	 */
	if (printpath(dst, sizeof dst, "%s/new/%s", path, name) != 0) {
		log_warn("%s: %s: printpath", a->name, path);
		goto error;
	}
	log_debug2("%s: linking .../%s to .../%s", a->name,
	    src + strlen(path) + 1, dst + strlen(path) + 1);
	if (link(src, dst) != 0) {
		if (errno == EEXIST) {
			if (unlink(src) != 0)
				log_fatal("unlink");
			cleanup_deregister(src);
			xfd = fd = -1;

			log_debug2("%s: link failed", a->name);
			goto restart;
		}
		log_warn("%s: %s: link(\"%s\")", a->name, src, dst);
		goto error;
	}

	/* Unlink the original tmp file. */
	log_debug2("%s: unlinking .../%s", a->name, src + strlen(path) + 1);
	if (unlink(src) != 0) {
		log_warn("%s: %s: unlink", a->name, src);
		goto error;
	}
	cleanup_deregister(src);

	/* Save the mail file as a tag. */
	add_tag(&m->tags, "mail_file", "%s", dst);

	if (path != NULL)
		xfree(path);
	return (DELIVER_SUCCESS);

error:
	if (fd != -1 || xfd != -1) {
		if (fd != -1)
			close(fd);
		if (unlink(src) != 0)
			log_fatal("unlink");
		cleanup_deregister(src);
	}
	if (path != NULL)
		xfree(path);
	return (DELIVER_FAILURE);
}

void
deliver_maildir_desc(struct actitem *ti, char *buf, size_t len)
{
	struct deliver_maildir_data	*data = ti->data;

	xsnprintf(buf, len, "maildir \"%s\"", data->path.str);
}
