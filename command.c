/* $Id: command.c,v 1.16 2006-11-27 23:05:38 nicm Exp $ */

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

#include <fcntl.h>
#include <paths.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

#include "fdm.h"

struct cmd *
cmd_start(const char *s, int in, int out, char *buf, size_t len, char **cause)
{
	struct cmd	*cmd;
	int	 	 fd_in[2], fd_out[2], fd_err[2];

	cmd = xmalloc(sizeof *cmd);
	cmd->pid = -1;

	fd_in[0] = fd_in[1] = -1;
	fd_out[0] = fd_out[1] = -1;
	fd_err[0] = fd_err[1] = -1;

	/* open child's stdin */
	if (in) {
		if (pipe(fd_in) != 0) {
			xasprintf(cause, "pipe: %s", strerror(errno));
			goto error;
		}
	} else {
		fd_in[0] = open(_PATH_DEVNULL, O_RDONLY, 0);
		if (fd_in[0] < 0) {
			xasprintf(cause, "open: %s", strerror(errno));
			goto error;
		}
	}

	/* open child's stdout */
	if (out) {
		if (pipe(fd_out) != 0) {
			xasprintf(cause, "pipe: %s", strerror(errno));
			goto error;
		}
	} else {
		fd_out[1] = open(_PATH_DEVNULL, O_WRONLY, 0);
		if (fd_out[1] < 0) {
			xasprintf(cause, "open: %s", strerror(errno));
			goto error;
		}
	}

	/* open child's stderr */
	if (pipe(fd_err) != 0) {
		xasprintf(cause, "pipe: %s", strerror(errno));
		goto error;
	}

	/* fork the child */
	switch (cmd->pid = fork()) {
	case -1:
		xasprintf(cause, "fork: %s", strerror(errno));
		goto error;
	case 0:
		/* child */
		if (fd_in[1] != -1)
			close(fd_in[1]);
		if (fd_out[0] != -1)
			close(fd_out[0]);
		close(fd_err[0]);

		if (dup2(fd_in[0], STDIN_FILENO) == -1)
			fatal("dup2(stdin)");
		if (dup2(fd_out[1], STDOUT_FILENO) == -1)
			fatal("dup2(stdout)");
		if (dup2(fd_err[1], STDERR_FILENO) == -1)
			fatal("dup2(stderr)");

		execl(_PATH_BSHELL, "sh", "-c", s, (char *) NULL);
		fatal("execl");
	}

	/* parent */
	close(fd_in[0]);
	fd_in[0] = -1;
	close(fd_out[1]);
	fd_out[1] = -1;
	close(fd_err[1]);
	fd_err[1] = -1;

	/* create ios */
	if (fd_in[1] != -1 && buf != NULL && len > 0) {
		cmd->io_in = io_create(fd_in[1], NULL, IO_LF);
		/* write the buffer directly, without copying */
		io_writefixed(cmd->io_in, buf, len);
		cmd->io_in->flags &= ~IO_RD;
	}
	if (fd_out[0] != -1) {
		cmd->io_out = io_create(fd_out[0], NULL, IO_LF);
		cmd->io_out->flags &= ~IO_WR;
	} else
		cmd->io_out = NULL;
	if (fd_err[0] != -1) {
		cmd->io_err = io_create(fd_err[0], NULL, IO_LF);
		cmd->io_err->flags &= ~IO_WR;
	} else
		cmd->io_err = NULL;

	return (cmd);

error:
	if (cmd->pid != -1)
		kill(cmd->pid, SIGTERM);

	if (fd_in[0] != -1)
		close(fd_in[0]);
	if (fd_in[1] != -1)
		close(fd_in[1]);
	if (fd_out[0] != -1)
		  close(fd_out[0]);
	if (fd_out[1] != -1)
		close(fd_out[1]);
	if (fd_err[0] != -1)
		close(fd_err[0]);
	if (fd_err[1] != -1)
		close(fd_err[1]);

	xfree(cmd);
	return (NULL);
}

int
cmd_poll(struct cmd *cmd, char **out, char **err, char **cause)
{
	struct io	*io, *ios[2];

	/* retrieve a line if possible */
	*out = *err = NULL;
	if (cmd->io_out != NULL)
		*out = io_readline(cmd->io_out);
	if (cmd->io_err != NULL)
		*err = io_readline(cmd->io_err);
	if (*out != NULL || *err != NULL)
		return (0);

	/* if anything is open, try and poll it */
	if (cmd->io_in != NULL || cmd->io_out != NULL || cmd->io_err != NULL) {
		ios[0] = cmd->io_in;
		ios[1] = cmd->io_out;
		ios[2] = cmd->io_err;
		switch (io_polln(ios, 3, &io, cause)) {
		case -1:
			return (1);
		case 0:
			/* if the closed io is empty, free it */
			if (io == cmd->io_out && IO_RDSIZE(cmd->io_out) == 0) {
				io_close(cmd->io_out);
				io_free(cmd->io_out);
				cmd->io_out = NULL;
			}
			if (io == cmd->io_err && IO_RDSIZE(cmd->io_err) == 0) {
				io_close(cmd->io_err);
				io_free(cmd->io_err);
				cmd->io_err = NULL;
			}
			break;
		}
	}

	/* close stdin if it is done */
	if (cmd->io_in != NULL && IO_WRSIZE(cmd->io_in) == 0) {
		io_close(cmd->io_in);
		io_free(cmd->io_in);
		cmd->io_in = NULL;
	}


	/* check if the child is still alive */
	if (cmd->pid != -1) {
		switch (waitpid(cmd->pid, &cmd->status, WNOHANG)) {
		case -1:
			if (errno == ECHILD)
				break;
			xasprintf(cause, "waitpid: %s", strerror(errno));
			return (1);
		case 0:
			break;
		default:
			cmd->pid = -1;
			break;
		}
	}

	/* if the child isn't dead, or there is data left in the buffers,
	   return with 0 now to get called again */
	if (cmd->pid != -1)
		return (0);
	if (cmd->io_out != NULL && IO_RDSIZE(cmd->io_out) > 0)
		return (0);
	if (cmd->io_err != NULL && IO_RDSIZE(cmd->io_err) > 0)
		return (0);

	/* child is dead, everything is empty. sort out what to return */
	if (WIFSIGNALED(cmd->status)) {
		xasprintf(cause, "child got signal: %d", WTERMSIG(cmd->status));
		return (1);
	}
	if (!WIFEXITED(cmd->status)) {
		xasprintf(cause, "child didn't exit normally");
		return (1);
	}
	cmd->status = WEXITSTATUS(cmd->status);
	return (-1 - cmd->status);
}

void
cmd_free(struct cmd *cmd)
{
	if (cmd->pid != -1)
		kill(cmd->pid, SIGTERM);

	if (cmd->io_in != NULL) {
		io_close(cmd->io_in);
		io_free(cmd->io_in);
	}
	if (cmd->io_out != NULL) {
		io_close(cmd->io_out);
		io_free(cmd->io_out);
	}
	if (cmd->io_err != NULL) {
		io_close(cmd->io_err);
		io_free(cmd->io_err);
	}

	xfree(cmd);
}
