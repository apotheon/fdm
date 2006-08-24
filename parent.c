/* $Id: parent.c,v 1.1 2006-08-24 12:38:01 nicm Exp $ */

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
#include <sys/wait.h>

#include <unistd.h>

#include "fdm.h"

int	perform_actions(struct account *, struct mail *, struct rule *);

int
parent(int fd, pid_t pid)
{
	struct io	*io;
	struct msg	 msg;
	int		 status, error;

	io = io_create(fd, NULL, IO_LF);
	log_debug("parent: started, pid %ld", (long) getpid());

	setproctitle("parent");

	do {
		if (io_wait(io, sizeof msg) != 0) 
			fatalx("parent: io_wait error");
		if (io_read2(io, &msg, sizeof msg) != 0)
			fatalx("parent: io_read2 error");

		log_debug("parent: got message %d", msg.type);

		switch (msg.type) {
		case MSG_DELIVER:
			if (io_wait(io, msg.mail.size) != 0) 
				fatalx("parent: io_wait error"); 
			msg.mail.base = io_read(io, msg.mail.size);
			if (msg.mail.base == NULL)
				fatalx("parent: io_read error"); 
			msg.mail.data = msg.mail.base;

			trim_from(&msg.mail);
			error = perform_actions(msg.acct, &msg.mail, msg.rule);
			free_mail(&msg.mail);

			msg.type = MSG_DONE;
			msg.error = error;
			io_write(io, &msg, sizeof msg);
			if (io_flush(io) != 0)
				fatalx("parent: io_flush error");
			break;
		case MSG_DONE:
			fatalx("parent: unexpected message");
		case MSG_EXIT:
			break;
		}

	} while (msg.type != MSG_EXIT);

	io_free(io);

#ifdef DEBUG
	xmalloc_dump("parent");
#endif 

	if (waitpid(pid, &status, 0) == -1)
		fatal("waitpid");
	if (!WIFEXITED(status))
		return (1);
	return (WEXITSTATUS(status));
}

int
perform_actions(struct account *a, struct mail *m, struct rule *r)
{
	struct action	*t;
	u_int		 i;
	int		 status;
	uid_t		 uid;
	pid_t		 pid;
	
	for (i = 0; i < ARRAY_LENGTH(r->actions); i++) {
		t = ARRAY_ITEM(r->actions, i);
		if (t->deliver->deliver == NULL)
			continue;
		log_debug2("%s: action %s", a->name, t->name);
		
		if (geteuid() != 0) {
			log_debug2("%s: not root. using current user", a->name);
			/* do the delivery without forking */
			if (t->deliver->deliver(a, t, m) != 0)
				return (1);
			continue;
		}
		
		pid = fork();
		if (pid == -1) {
			log_warn("%s: fork", a->name);
			return (1);
		}
		if (pid != 0) {
			/* parent process. wait for child */
			log_debug2("%s: forked. child pid is %ld", a->name, 
			    (long) pid);
			if (waitpid(pid, &status, 0) == -1)
				fatal("waitpid");
			if (!WIFEXITED(status)) {
				log_warnx("%s: child didn't exit normally",
				    a->name);
				return (1);
			}
			status = WEXITSTATUS(status);
			if (status != 0) {
				log_warnx("%s: child failed, exit code %d",
				    a->name, status);
				return (1);
			}
			continue;
		}
		
		/* figure out the user to use */
		uid = t->uid != 0 ? t->uid : r->uid;
		if (uid == 0)
			uid = conf.def_user;

		/* child process. change user and group */
		log_debug("%s: delivering using user %lu", a->name, 
		    (u_long) uid);
		if (dropto(uid, NULL) != 0) {
			log_warnx("%s: can't drop privileges", a->name);
			_exit(1);
		}
		setproctitle("deliver");

		/* refresh user and home */
		fill_info(NULL);	
		log_debug2("%s: user is: %s, home is: %s", a->name, 
		    conf.info.user, conf.info.home);

		/* do the delivery */
		if (t->deliver->deliver(a, t, m) != 0)
			_exit(1);
		_exit(0);
	}

	return (0);
}
