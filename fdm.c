/* $Id: fdm.c,v 1.65 2006-11-11 15:07:44 nicm Exp $ */

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
#include <sys/socket.h>
#include <sys/stat.h>

#include <errno.h>
#include <fcntl.h>
#include <fnmatch.h>
#include <grp.h>
#include <limits.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "fdm.h"

#ifdef DEBUG
const char		*malloc_options = "AFGJX";
#endif

extern FILE		*yyin;
extern int 		 yyparse(void);

int			 load_conf(void);
void			 usage(void);

struct conf		 conf;

int
load_conf(void)
{
        yyin = fopen(conf.conf_file, "r");
        if (yyin == NULL)
                return (1);

        yyparse();

        fclose(yyin);

        return (0);
}

void
fill_info(const char *home)
{
	struct passwd	*pw;
	uid_t		 uid;
	char		 host[MAXHOSTNAMELEN];

	if (conf.info.uid != NULL) {
		xfree(conf.info.uid);
		conf.info.uid = NULL;
	}
	if (conf.info.user != NULL) {
		xfree(conf.info.user);
		conf.info.user = NULL;
	}
	if (conf.info.home != NULL) {
		xfree(conf.info.home);
		conf.info.home = NULL;
	}

	if (conf.info.host == NULL) {
		if (gethostname(host, sizeof host) != 0)
			fatal("gethostname");
		conf.info.host = xstrdup(host);
	}

	if (home != NULL && *home != '\0')
		conf.info.home = xstrdup(home);

	uid = getuid();
	xasprintf(&conf.info.uid, "%lu", (u_long) uid);
	pw = getpwuid(uid);
	if (pw != NULL) {
		if (conf.info.home == NULL) {
			if (pw->pw_dir != NULL && *pw->pw_dir != '\0')
				conf.info.home = xstrdup(pw->pw_dir);
			else
				conf.info.home = xstrdup(".");
		}
		if (pw->pw_name != NULL && *pw->pw_name != '\0')
			conf.info.user = xstrdup(pw->pw_name);
	}
	endpwent();
	if (conf.info.user == NULL) {
		conf.info.user = xstrdup(conf.info.uid);
		log_warn("can't find name for user %lu", (u_long) uid);
	}
}

int
dropto(uid_t uid)
{
	struct passwd	*pw;
	gid_t		 gid;

	if (uid == 0)
		return (0);

	pw = getpwuid(uid);
	if (pw == NULL) {
		endpwent();
		errno = ESRCH;
		return (1);
	}
	gid = pw->pw_gid;
	endpwent();

	if (setgroups(1, &gid) != 0 ||
	    setresgid(gid, gid, gid) != 0 || setresuid(uid, uid, uid) != 0)
		return (1);

	return (0);
}

int
check_incl(char *name)
{
	u_int	i;

	if (ARRAY_EMPTY(&conf.incl))
		return (1);

	for (i = 0; i < ARRAY_LENGTH(&conf.incl); i++) {
		if (name_match(ARRAY_ITEM(&conf.incl, i, char *), name))
			return (1);
	}

	return (0);
}

int
check_excl(char *name)
{
	u_int	i;

	if (ARRAY_EMPTY(&conf.excl))
		return (0);

	for (i = 0; i < ARRAY_LENGTH(&conf.excl); i++) {
		if (name_match(ARRAY_ITEM(&conf.excl, i, char *), name))
			return (1);
	}

	return (0);
}

__dead void
usage(void)
{
	printf("usage: %s [-lmnv] [-f conffile] [-h show|clear] [-u user] "
	    "[-a name] [-x name] [fetch|poll]\n", __progname);
        exit(1);
}

int
main(int argc, char **argv)
{
        int		 opt, fds[2], lockfd, rc;
	u_int		 i;
	enum cmd         cmd = CMD_NONE;
	enum histcmd	 histcmd = HISTCMD_NONE;
	const char	*errstr;
	char		 tmp[512];
	const char	*proxy = NULL;
	char		*user = NULL, *lock = NULL, *hist = NULL;
	long		 n;
	pid_t		 pid;
	struct passwd	*pw;
	struct stat	 sb;
	FILE		*histf = NULL;

	memset(&conf, 0, sizeof conf);
	TAILQ_INIT(&conf.accounts);
	TAILQ_INIT(&conf.rules);
	TAILQ_INIT(&conf.actions);
	conf.max_size = DEFMAILSIZE;
	conf.lock_types = LOCK_FLOCK;

	log_init(1);

	ARRAY_INIT(&conf.incl);
	ARRAY_INIT(&conf.excl);

        while ((opt = getopt(argc, argv, "a:f:h:mlnu:vx:")) != EOF) {
                switch (opt) {
		case 'a':
			ARRAY_ADD(&conf.incl, optarg, char *);
			break;
                case 'f':
                        conf.conf_file = xstrdup(optarg);
                        break;
		case 'h':
			if (strncmp(optarg, "show", strlen(optarg)) == 0)
				histcmd = HISTCMD_SHOW;
			else if (strncmp(optarg, "clear", strlen(optarg)) == 0)
				histcmd = HISTCMD_CLEAR;
			else
				usage();
			break;
		case 'l':
			conf.syslog = 1;
			break;
		case 'm':
			conf.allow_many = 1;
			break;
		case 'n':
			conf.check_only = 1;
			break;
		case 'u':
			user = optarg;
			break;
                case 'v':
                        conf.debug++;
                        break;
		case 'x':
			ARRAY_ADD(&conf.excl, optarg, char *);
			break;
                case '?':
                default:
                        usage();
                }
        }
	argc -= optind;
	argv += optind;
	if (conf.check_only || histcmd != HISTCMD_NONE) {
		if (argc != 0)
			usage();
	} else {
		if (argc != 1)
			usage();
		if (strcmp(argv[0], "poll") == 0)
			cmd = CMD_POLL;
		else if (strcmp(argv[0], "fetch") == 0)
			cmd = CMD_FETCH;
		else
			usage();
	}

	/* check the user */
	if (user != NULL) {
		pw = getpwnam(user);
		if (pw == NULL) {
			n = strtonum(user, 0, UID_MAX, &errstr);
			if (errstr != NULL) {
				if (errno == ERANGE) {
					log_warnx("invalid uid: %s", user);
					exit(1);
				}
			} else
				pw = getpwuid((uid_t) n);
			if (pw == NULL) {
				log_warnx("unknown user: %s", user);
				exit(1);
			}
		}
		conf.def_user = pw->pw_uid;
		endpwent();
	}

	/* start logging to syslog if necessary */
	log_init(!conf.syslog);
	log_debug("version is: %s " BUILD, __progname);

	/* save the home dir and misc user info */
	fill_info(getenv("HOME"));
	log_debug("user is: %s, home is: %s", conf.info.user, conf.info.home);

	/* find the config file */
	if (conf.conf_file == NULL) {
		/* if no file specified, try ~ then /etc */
		xasprintf(&conf.conf_file, "%s/%s", conf.info.home, CONFFILE);
		if (access(conf.conf_file, R_OK) != 0) {
			xfree(conf.conf_file);
			conf.conf_file = xstrdup(SYSCONFFILE);
		}
	}
	log_debug("loading configuration from %s", conf.conf_file);
	if (stat(conf.conf_file, &sb) == -1) {
                log_warn("%s", conf.conf_file);
		exit(1);
	}
	if (geteuid() != 0 && (sb.st_mode & (S_IROTH|S_IWOTH)) != 0)
		log_warnx("%s: world readable or writable", conf.conf_file);
        if (load_conf() != 0) {
                log_warn("%s", conf.conf_file);
		exit(1);
	}
	log_debug("configuration loaded");

	/* find history file and open it */
	hist = conf.hist_file;
	if (hist == NULL) {
		if (geteuid() == 0)
			hist = xstrdup(SYSHISTFILE);
		else
			xasprintf(&hist, "%s/%s", conf.info.home, HISTFILE);
	}
	if (*hist != '\0') {
		log_debug2("history file is: %s", hist);
		if ((histf = fopen(hist, "r+")) == NULL) {
			if ((histf = fopen(hist, "w+")) == NULL)
				log_warn("%s", hist);
		}
	} else
		log_debug2("history is disabled");
	conf.hist_file = hist;

	/* do history command if specified */
	if (histcmd != HISTCMD_NONE)
		exit(do_hist(histcmd, histf));

	/* fill proxy */
	proxy = getenv("http_proxy");
	if (proxy != NULL && *proxy != '\0') {
		if (conf.proxy != NULL) {
			xfree(conf.proxy->server.host);
			xfree(conf.proxy->server.port);
			if (conf.proxy->user != NULL)
				xfree(conf.proxy->user);
			if (conf.proxy->pass != NULL)
				xfree(conf.proxy->pass);
		}

		/* getenv's return buffer is read-only */
		if ((conf.proxy = getproxy(proxy)) == NULL) {
			log_warnx("invalid proxy: %s", proxy);
			exit(1);
		}
	}

	/* print proxy info */
	if (conf.proxy != NULL) {
		log_debug("using proxy: %s on %s:%s",
		    conf.proxy->type == PROXY_HTTP ? "HTTP" : "SOCKS5",
		    conf.proxy->server.host, conf.proxy->server.port);
	}

	/* print some locking info */
	*tmp = '\0';
	if (conf.lock_types == 0)
		strlcpy(tmp, "none", sizeof tmp);
	else {
		if (conf.lock_types & LOCK_FCNTL)
			strlcat(tmp, "fcntl ", sizeof tmp);
		if (conf.lock_types & LOCK_FLOCK)
			strlcat(tmp, "flock ", sizeof tmp);
		if (conf.lock_types & LOCK_DOTLOCK)
			strlcat(tmp, "dotlock ", sizeof tmp);
	}
	log_debug("locking using: %s", tmp);

	/* initialise and print headers and domains */
	if (conf.headers == NULL) {
		conf.headers = xmalloc(sizeof (struct headers));
		ARRAY_INIT(conf.headers);
		ARRAY_ADD(conf.headers, xstrdup("to"), char *);
		ARRAY_ADD(conf.headers, xstrdup("cc"), char *);
	}
	xsnprintf(tmp, sizeof tmp, "headers are: ");
	for (i = 0; i < ARRAY_LENGTH(conf.headers); i++) {
		strlcat(tmp, ARRAY_ITEM(conf.headers, i, char *), sizeof tmp);
		strlcat(tmp, " ", sizeof tmp);
	}
	log_debug("%s", tmp);
	if (conf.domains == NULL) {
		conf.domains = xmalloc(sizeof (struct domains));
		ARRAY_INIT(conf.domains);
		ARRAY_ADD(conf.domains, conf.info.host, char *);
	}
	xsnprintf(tmp, sizeof tmp, "domains are: ");
	for (i = 0; i < ARRAY_LENGTH(conf.domains); i++) {
		strlcat(tmp, ARRAY_ITEM(conf.domains, i, char *), sizeof tmp);
		strlcat(tmp, " ", sizeof tmp);
	}
	log_debug("%s", tmp);

	/* if -n, bail now, otherwise check there is something to work with */
	if (conf.check_only)
		exit(0);
        if (TAILQ_EMPTY(&conf.accounts)) {
                log_warnx("no accounts specified");
		exit(1);
	}
        if (cmd == CMD_FETCH && TAILQ_EMPTY(&conf.rules)) {
                log_warnx("no rules specified");
		exit(1);
	}

	if (geteuid() == 0) {
		pw = getpwnam(CHILDUSER);
		if (pw == NULL) {
			log_warnx("can't find user: %s", CHILDUSER);
			exit(1);
		}
		conf.uid = pw->pw_uid;
		endpwent();

		if (conf.def_user == 0) {
			log_warnx("no default user specified");
			exit(1);
		}
	}

	/* check lock file */
	lock = conf.lock_file;
	if (lock == NULL) {
		if (geteuid() == 0)
			lock = xstrdup(SYSLOCKFILE);
		else
			xasprintf(&lock, "%s/%s", conf.info.home, LOCKFILE);
	}
	if (*lock != '\0' && !conf.allow_many) {
		lockfd = open(lock, O_WRONLY|O_CREAT|O_EXCL, S_IRUSR|S_IWUSR);
		if (lockfd == -1 && errno == EEXIST) {
			log_warnx("already running (%s exists)", lock);
			exit(1);
		} else if (lockfd == -1) {
			log_warn("%s: open", lock);
			exit(1);
		}
		close(lockfd);
	}
	conf.lock_file = lock;
	    
#ifdef DEBUG
	xmalloc_clear();
#endif

	if (socketpair(AF_UNIX, SOCK_STREAM, PF_UNSPEC, fds) != 0)
		fatal("socketpair");
	switch (pid = fork()) {
	case -1:
		fatal("fork");
	case 0:
		close(fds[0]);
		_exit(child(fds[1], cmd, histf));
	default:
		if (histf != NULL)
			fclose(histf);
		close(fds[1]);
		rc = parent(fds[0], pid);
		if (*conf.lock_file != '\0' && !conf.allow_many)
			unlink(conf.lock_file);
		exit(rc);
	}
}
