/* $Id: fdm.h,v 1.92 2006-11-17 17:50:58 nicm Exp $ */

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

#ifndef FDM_H
#define FDM_H

#include <sys/types.h>
#include <sys/queue.h>

#include <stdarg.h>
#include <regex.h>

#include <openssl/ssl.h>
#include <openssl/err.h>

#define CHILDUSER	"_fdm"
#define CONFFILE	".fdm.conf"
#define SYSCONFFILE	"/etc/fdm.conf"
#define LOCKFILE	".fdm.lock"
#define SYSLOCKFILE	"/var/run/fdm.lock"
#define MAXMAILSIZE	INT_MAX
#define DEFMAILSIZE	(1 * 1024 * 1024 * 1024)	/* 1 GB */
#define LOCKSLEEPTIME	2
#define MAXNAMESIZE	32

extern char	*__progname;

/* Linux compatibility bullshit. */
#ifndef UID_MAX
#define UID_MAX UINT_MAX
#endif

#ifndef __dead
#define __dead __attribute__ ((noreturn))
#endif

#ifndef TAILQ_HEAD_INITIALIZER
#define TAILQ_HEAD_INITIALIZER(head)					\
	{ NULL, &(head).tqh_first }
#endif
#ifndef TAILQ_FIRST
#define TAILQ_FIRST(head) (head)->tqh_first
#endif
#ifndef TAILQ_END
#define TAILQ_END(head) NULL
#endif
#ifndef TAILQ_NEXT
#define TAILQ_NEXT(elm, field) ((elm)->field.tqe_next)
#endif
#ifndef TAILQ_FOREACH
#define TAILQ_FOREACH(var, head, field)					\
	for ((var) = TAILQ_FIRST(head);					\
	     (var) != TAILQ_END(head);				 	\
	     (var) = TAILQ_NEXT(var, field))
#endif
#ifndef TAILQ_EMPTY
#define TAILQ_EMPTY(head) (TAILQ_FIRST(head) == TAILQ_END(head))
#endif

/* Array macros. */
#define ARRAY_DECLARE(n, c)						\
	struct n {							\
		c	*list;						\
		u_int	 num;						\
	}
#define ARRAY_INIT(a) do {						\
	(a)->num = 0;							\
	(a)->list = NULL;      						\
} while (0)
#define ARRAY_ADD(a, s, c) do {						\
	(a)->list = xrealloc((a)->list, (a)->num + 1, sizeof (c));	\
	((c *) (a)->list)[(a)->num] = s;				\
	(a)->num++;							\
} while (0)
#define ARRAY_REMOVE(a, i, c) do {					\
	if (((u_int) (i)) >= (a)->num) {				\
		log_warnx("ARRAY_REMOVE: bad index: %u, at %s:%d",	\
		    i, __FILE__, __LINE__);				\
		exit(1);						\
	}								\
	if (i < (a)->num - 1) {						\
		size_t	 size = sizeof (c);				\
		c 	*ptr = (a)->list + (i) * size;			\
		memmove(ptr, ptr + size, size * ((a)->num - (i) - 1)); 	\
	}								\
	(a)->num--;							\
        if ((a)->num == 0) {						\
		xfree((a)->list);					\
		(a)->list = NULL;					\
	} else								\
		(a)->list = xrealloc((a)->list, (a)->num, sizeof (c));	\
} while (0)
#define ARRAY_CONCAT(a, b, c) {						\
	size_t	size = sizeof (c);					\
	(a)->list = xrealloc((a)->list, (a)->num + (b)->num, size);	\
	memcpy((a)->list + (a)->num, (b)->list, (b)->num * size);  	\
	(a)->num += (b)->num;						\
} while (0)
#define ARRAY_EMPTY(a) ((a) == NULL || (a)->num == 0)
#define ARRAY_LENGTH(a) ((a)->num)
#define ARRAY_ITEM(a, n, c) (((c *) (a)->list)[n])
#define ARRAY_FREE(a) do {						\
	if ((a)->list != NULL)						\
		xfree((a)->list);					\
	ARRAY_INIT(a);							\
} while (0)
#define ARRAY_FREEALL(a) do {						\
	ARRAY_FREE(a);							\
	xfree(a);							\
} while (0)

/* Definition to shut gcc up about unused arguments in a few cases. */
#define unused __attribute__ ((unused))

/* Attribute to make gcc check printf-like arguments. */
#define printflike1 __attribute__ ((format (printf, 1, 2)))
#define printflike2 __attribute__ ((format (printf, 2, 3)))
#define printflike3 __attribute__ ((format (printf, 3, 4)))

/* Ensure buffer size. */
#define ENSURE_SIZE(buf, len, size) do {				\
	(buf) = ensure_size(buf, &(len), 1, size);			\
} while (0)
#define ENSURE_SIZE2(buf, len, nmemb, size) do {			\
	(buf) = ensure_size(buf, &(len), nmemb, size);			\
} while (0)
#define ENSURE_FOR(buf, len, now, size) do {				\
	(buf) = ensure_for(buf, &(len), now, 1, size);			\
} while (0)
#define ENSURE_FOR2(buf, len, now, nmemb, size) do {			\
	(buf) = ensure_for(buf, &(len), now, nmemb, size);		\
} while (0)

/* Valid email address chars. */
#define isaddr(c) ( 							\
	((c) >= 'a' && (c) <= 'z') || 					\
	((c) >= 'A' && (c) <= 'Z') ||					\
	((c) >= '0' && (c) <= '9') ||					\
	(c) == '&' || (c) == '*' || (c) == '+' || (c) == '?' ||	 	\
	(c) == '-' || (c) == '.' || (c) == '=' || (c) == '/' ||		\
	(c) == '^' || (c) == '{' || (c) == '}' || (c) == '~' || 	\
	(c) == '_' || (c) == '@' || (c) == '\'')

/* Account name match. */
#define name_match(p, n) (fnmatch(p, n, 0) == 0)

/* Macros in configuration file. */
struct macro {
	char			 name[MAXNAMESIZE];
	union {
		long long	 number;
		char		*string;
	} value;
	enum {
		MACRO_NUMBER,
		MACRO_STRING
	} type;

	TAILQ_ENTRY(macro)	entry;
};
TAILQ_HEAD(macros, macro);

/* Valid macro name chars. */
#define ismacrofirst(c) (						\
	((c) >= 'a' && (c) <= 'z') || ((c) >= 'A' && (c) <= 'Z'))
#define ismacro(c) (							\
	((c) >= 'a' && (c) <= 'z') || ((c) >= 'A' && (c) <= 'Z') ||	\
	((c) >= '0' && (c) <= '9') || (c) == '_' || (c) == '-')

/* Command-line commands. */
enum cmd {
	CMD_NONE = 0,
	CMD_POLL,
	CMD_FETCH
};

/* Server description. */
struct server {
	char		*host;
	char		*port;
	struct addrinfo	*ai;
	int		 ssl;
};

/* Proxy type. */
enum proxytype {
	PROXY_HTTP,
	PROXY_HTTPS,
	PROXY_SOCKS5
};

/* Proxy definition. */
struct proxy {
	enum proxytype	 type;
	char		*user;
	char		*pass;
	struct server	 server;
};

/* A single mail. */
struct mail {
	char	*base;

	char	*data;
	size_t	 size;		/* size of mail */
	size_t	 space;		/* size of malloc'd area */

	char	*from;		/* from line */

	size_t	*wrapped;	/* list of wrapped lines */

	ssize_t	 body;		/* offset of body */
};

/* Privsep message types. */
enum msgtype {
	MSG_DELIVER,
	MSG_EXIT,
	MSG_DONE
};

/* Privsaep message data. */
struct msgdata {
	int	 	 error;
	struct mail	 mail;
	
	/* these only work so long as they aren't moved in either process */
	struct rule	*rule;
	struct account	*account;
};

/* Privsep message. */
struct msg {
	u_int		 n;

	enum msgtype	 type;
	size_t		 size;

	struct msgdata	 data;
};

/* Account entry. */
struct account {
	char			 name[MAXNAMESIZE];

	int			 disabled;
	struct fetch		*fetch;
	void			*data;

	TAILQ_ENTRY(account)	 entry;
};

/* Action definition. */
struct action {
	char			 name[MAXNAMESIZE];

	struct users		*users;
	int			 find_uid;

	struct deliver		*deliver;
	void			*data;

	TAILQ_ENTRY(action)	 entry;
};

/* Accounts array. */
ARRAY_DECLARE(accounts, char *);

/* Actions array. */
ARRAY_DECLARE(actions, struct action *);

/* Match areas. */
enum area {
	AREA_BODY,
	AREA_HEADERS,
	AREA_ANY
};

/* Expression operators. */
enum exprop {
	OP_NONE,
	OP_AND,
	OP_OR
};

/* Expression item. */
struct expritem {
	struct match		*match;
	void			*data;

	enum exprop		 op;
	int			 inverted;

	TAILQ_ENTRY(expritem)	 entry;
};

/* Expression strut. */
TAILQ_HEAD(expr, expritem);

/* Rule types. */
enum ruletype {
	RULE_EXPRESSION,
	RULE_ALL,
	RULE_MATCHED,
	RULE_UNMATCHED
};

/* Rule entry. */
struct rule {
	enum ruletype		 type;
	struct expr		*expr;

	struct users		*users;
	int			 find_uid;	/* find uids from headers */

	int			 stop;		/* stop matching at this rule */

	struct actions		*actions;
	struct accounts		*accounts;

	TAILQ_ENTRY(rule)	 entry;
};

/* Match functions. */
struct match {
	const char		*name;

	int			 (*match)(struct account *, struct mail *, 
			    	      struct expritem *);
	char 			*(*desc)(struct expritem *);
};

/* Match regexp data. */
struct regexp_data {
	char			*re_s;
	regex_t			 re;

	enum area	 	 area;
};

/* Match command data. */
struct command_data {
	char			*cmd;
	int			 pipe;		/* pipe mail to command */

	char			*re_s;		/* NULL to not check */
	regex_t			 re;
	int			 ret;		/* -1 to not check */
};

/* Deliver return codes. */
#define DELIVER_SUCCESS 0
#define DELIVER_FAILURE 1

/* Poll return codes. */
#define POLL_SUCCESS FETCH_SUCCESS
#define POLL_ERROR FETCH_ERROR

/* Fetch return codes. */
#define FETCH_SUCCESS 0
#define FETCH_ERROR 1
#define FETCH_OVERSIZE 2
#define FETCH_COMPLETE 3

/* Fetch functions. */
struct fetch {
	const char	*name;
	const char	*port;

	int	 	 (*connect)(struct account *);
	int 		 (*poll)(struct account *, u_int *);
	int	 	 (*fetch)(struct account *, struct mail *);
	int		 (*delete)(struct account *);
	void		 (*error)(struct account *);
	int		 (*disconnect)(struct account *);
};

/* Deliver functions. */
struct deliver {
	const char	*name;

	int	 	 (*deliver)(struct account *, struct action *, 
			     struct mail *);
};

/* Lock types. */
#define LOCK_FCNTL 0x1
#define LOCK_FLOCK 0x2
#define LOCK_DOTLOCK 0x4

/* Domains array. */
ARRAY_DECLARE(domains, char *);

/* Headers array. */
ARRAY_DECLARE(headers, char *);

/* Users array. */
ARRAY_DECLARE(users, char *);

/* Configuration settings. */
struct conf {
	int 			 debug;
	int			 syslog;

	uid_t			 uid;

	struct accounts	 	 incl;
	struct accounts		 excl;

	struct proxy		*proxy;

	struct domains		*domains; /* domains to look for with users */
	struct headers		*headers; /* headers to search for users */

	struct {
		char		*home;
		char		*user;
		char		*uid;
		char		*host;
	} info;

	char			*conf_file;
	char			*lock_file;
	int			 check_only;
	int			 allow_many;

	size_t			 max_size;
	int		         del_big;
	u_int			 lock_types;
	uid_t			 def_user;

	TAILQ_HEAD(, account)	 accounts;
 	TAILQ_HEAD(, action)	 actions;
 	TAILQ_HEAD(, rule)	 rules;
};
extern struct conf		 conf;

/* Shorthand for the ridiculous call to get the SSL error. */
#define SSL_err() (ERR_error_string(ERR_get_error(), NULL))

/* Limits at which to fail. */
#define IO_MAXLINELEN (1024 * 1024) 		/* 1 MB */
#define IO_MAXBUFFERLEN (1024 * 1024 * 1024) 	/* 1 GB */

/* IO line endings. */
#define IO_CRLF "\r\n"
#define IO_CR   "\r"
#define IO_LF   "\n"

/* Amount to attempt to append to the buffer each time. */
#define IO_BLOCKSIZE 16384

/* Initial line buffer length. */
#define IO_LINESIZE 256

/* Amount to poll after in io_update. */
#define IO_FLUSHSIZE (8 * IO_BLOCKSIZE)

/* IO structure. */
struct io {
	int		 fd;
	int		 dup_fd;	/* dup all data to this fd */
	SSL		*ssl;

	int		 closed;
	char		*error;
	int		 need;

	char		*rbase;		/* buffer start */
	size_t		 rspace;	/* total size of buffer */
	size_t		 rsize;		/* amount of data available */
	size_t		 roff;		/* base of data in buffer */

	char		*wbase;		/* buffer start */
	size_t		 wspace;	/* total size of buffer */
	size_t		 wsize;		/* size of data currently in buffer */

	const char	*eol;
};

/* Fetch stdin data. */
struct stdin_data {
	struct io	*io;

	int		 complete;
};

/* Fetch pop3 states. */
enum pop3_state {
	POP3_CONNECTING,
	POP3_USER,
	POP3_PASS,
	POP3_STAT,
	POP3_LIST,
	POP3_RETR,
	POP3_LINE,
	POP3_DONE,
	POP3_QUIT
};

/* Fetch pop3 data. */
struct pop3_data {
	char		*user;
	char		*pass;

	struct server	 server;

	enum pop3_state	 state;
	u_int		 cur;
	u_int		 num;

	struct io	*io;
};

/* Fetch pop3 macros. */
#define pop3_isOK(s) (strncmp(s, "+OK", 3) == 0)
#define pop3_isERR(s) (strncmp(s, "+ERR", 4) == 0)

/* Fetch imap states. */
enum imap_state {
	IMAP_CONNECTING,
	IMAP_USER,
	IMAP_PASS,
	IMAP_LOGIN,
	IMAP_SELECT,
	IMAP_SELECTWAIT,
	IMAP_SIZE,
	IMAP_LINE,
	IMAP_LINEWAIT,
	IMAP_LINEWAIT2,
	IMAP_DONE,
	IMAP_CLOSE,
	IMAP_LOGOUT
};

/* Fetch imap data. */
struct imap_data {
	char			*user;
	char			*pass;
	char			*folder;

	struct server		 server;

	enum imap_state	 	 state;
	int			 tag;
	u_int		 	 cur;
	u_int		 	 num;

	struct io		*io;
};

/* Deliver smtp states. */
enum smtp_state {
	SMTP_CONNECTING,
	SMTP_HELO,
	SMTP_FROM,
	SMTP_TO,
	SMTP_DATA,
	SMTP_DONE,
	SMTP_QUIT
};

/* Deliver smtp data. */
struct smtp_data {
	struct server	 server;
	char		*to;
};

/* match-command.c */
extern struct match	 match_command;
 
/* match-regexp.c */
extern struct match	 match_regexp;

/* fetch-stdin.c */
extern struct fetch 	 fetch_stdin;

/* fetch-pop3.c */
extern struct fetch 	 fetch_pop3;

/* fetch-pop3s.c */
extern struct fetch 	 fetch_pop3s;

/* fetch-imap.c */
extern struct fetch 	 fetch_imap;

/* fetch-imaps.c */
extern struct fetch 	 fetch_imaps;

/* deliver-smtp.c */
extern struct deliver	 deliver_smtp;

/* deliver-pipe.c */
extern struct deliver 	 deliver_pipe;

/* deliver-drop.c */
extern struct deliver 	 deliver_drop;

/* deliver-maildir.c */
extern struct deliver 	 deliver_maildir;

/* deliver-mbox.c */
extern struct deliver 	 deliver_mbox;

/* deliver-write.c */
extern struct deliver 	 deliver_write;
int	 do_write(struct account *, struct action *, struct mail *, int);

/* deliver-append.c */
extern struct deliver 	 deliver_append;

/* deliver-rewrite.c */
extern struct deliver 	 deliver_rewrite;

#ifdef NO_STRTONUM
/* strtonum.c */
long long		 strtonum(const char *, long long, long long,
			     const char **);
#endif

#ifdef NO_STRLCPY
/* strlcpy.c */
size_t	 		 strlcpy(char *, const char *, size_t);
#endif

#ifdef NO_STRLCAT
/* strlcat.c */
size_t	 		 strlcat(char *, const char *, size_t);
#endif

/* fdm.c */
int			 dropto(uid_t);
int			 check_incl(char *);
int		         check_excl(char *);
void			 fill_info(const char *);

/* privsep.c */
int			 privsep_send(struct io *, struct msg *, void *,
			     size_t);
int			 privsep_recv(struct io *, struct msg *, void **buf,
			     size_t *);

/* child.c */
int			 child(int, enum cmd);

/* parent.c */
int			 parent(int, pid_t);

/* connect.c */
struct proxy 		*getproxy(const char *);
struct io 		*connectproxy(struct server *, struct proxy *,
			     const char *, char **);
struct io		*connectio(struct server *, const char *, char **);

/* mail.c */
void			 free_mail(struct mail *);
void			 resize_mail(struct mail *, size_t);
int			 openlock(char *, u_int, int, mode_t);
void			 closelock(int, char *, u_int);
void			 line_init(struct mail *, char **, size_t *);
void			 line_next(struct mail *, char **, size_t *);
char 			*find_header(struct mail *, const char *, size_t *);
struct users		*find_users(struct mail *);
char			*find_address(char *, size_t, size_t *);
void			 trim_from(struct mail *);
void			 make_from(struct mail *);
u_int			 fill_wrapped(struct mail *);
void			 set_wrapped(struct mail *, char);
void			 free_wrapped(struct mail *);

/* replace.c */
#define REPL_LEN 52
#define REPL_IDX(ch) /* LINTED */ 				\
	((ch >= 'a' || ch <= 'z') ? ch - 'a' :			\
	((ch >= 'A' || ch <= 'z') ? 26 + ch - 'A' : -1))
char			*replaceinfo(char *, struct account *, struct action *);
char 			*replace(char *, char *[52]);

/* io.c */
struct io		*io_create(int, SSL *, const char *);
void			 io_free(struct io *);
void			 io_close(struct io *);
int			 io_update(struct io *, char **);
int			 io_poll(struct io *, char **);
int			 io_read2(struct io *, void *, size_t);
void 			*io_read(struct io *, size_t);
void			 io_write(struct io *, const void *, size_t);
char 			*io_readline2(struct io *, char **, size_t *);
char 			*io_readline(struct io *);
void printflike2	 io_writeline(struct io *, const char *, ...);
void			 io_vwriteline(struct io *, const char *, va_list);
int			 io_flush(struct io *, char **);
int			 io_wait(struct io *, size_t, char **);

/* log.c */
void			 log_init(int);
void		    	 vlog(int, const char *, va_list);
void printflike1	 log_warn(const char *, ...);
void printflike1	 log_warnx(const char *, ...);
void printflike1	 log_info(const char *, ...);
void printflike1	 log_debug(const char *, ...);
void printflike1	 log_debug2(const char *, ...);
void printflike1	 log_debug3(const char *, ...);
__dead void		 fatal(const char *);
__dead void		 fatalx(const char *);

/* xmalloc.c */
#ifdef DEBUG
void			 xmalloc_clear(void);
void			 xmalloc_dump(const char *);
#endif
void			*ensure_size(void *, size_t *, size_t, size_t);
void			*ensure_for(void *, size_t *, size_t, size_t, size_t);
char			*xstrdup(const char *);
void			*xcalloc(size_t, size_t);
void			*xmalloc(size_t);
void			*xrealloc(void *, size_t, size_t);
void			 xfree(void *);
int printflike2		 xasprintf(char **, const char *, ...);
int printflike3	 	 xsnprintf(char *, size_t, const char *, ...);

#endif /* FDM_H */
