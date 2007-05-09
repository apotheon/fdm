/* $Id: fdm.h,v 1.267 2007-05-09 18:52:44 nicm Exp $ */

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

#include <sys/param.h>
#include <sys/cdefs.h>

#ifndef NO_QUEUE_H
#include <sys/queue.h>
#else
#include "compat/queue.h"
#endif

#include <dirent.h>
#include <signal.h>
#include <stdarg.h>
#include <stdint.h>
#include <regex.h>

#ifdef PCRE
#include <pcre.h>
#endif

#include <openssl/ssl.h>
#include <openssl/err.h>

#include "array.h"

#define CHILDUSER	"_fdm"
#define CONFFILE	".fdm.conf"
#define SYSCONFFILE	"/etc/fdm.conf"
#define LOCKFILE	".fdm.lock"
#define SYSLOCKFILE	"/var/run/fdm.lock"
#define MAXQUEUEVALUE	50
#define DEFMAILQUEUE	2
#define DEFMAILSIZE	(32 * 1024 * 1024)		/* 32 MB */
#define MAXMAILSIZE	(1 * 1024 * 1024 * 1024)	/*  1 GB */
#define MAXACTIONCHAIN	5
#define DEFTIMEOUT	(900 * 1000)
#define LOCKSLEEPTIME	10000
#define LOCKRETRIES	1000
#define MAXNAMESIZE	64
#define DEFUMASK	(S_IRWXG|S_IRWXO)
#define FILEMODE	(S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH)
#define DIRMODE		(S_IRWXU|S_IRWXG|S_IRWXO)

#define NOGRP	 	((gid_t) -1)
#define NOUSR		((uid_t) -1)

extern char	*__progname;

/* Linux compatibility bullshit. */
#ifndef UID_MAX
#define UID_MAX UINT_MAX
#endif
#ifndef GID_MAX
#define GID_MAX UINT_MAX
#endif

#ifndef INFTIM
#define INFTIM -1
#endif

#ifndef __dead
#define __dead __attribute__ ((__noreturn__))
#endif
#ifndef __packed
#define __packed __attribute__ ((__packed__))
#endif

#ifdef DEBUG
#define NFDS 64
#define COUNTFDS(s) do {						\
	int	fd_i, fd_n;						\
	fd_n = 0;							\
	for (fd_i = 0; fd_i < NFDS; fd_i++) {				\
		if (fcntl(fd_i, F_GETFL) != -1)				\
			fd_n++;						\
	}								\
	log_debug2("%s: %d file descriptors in use", s, fd_n);		\
} while (0)
#endif

/* Convert a file mode. */
#define MODE(m) \
	(m & S_IRUSR ? 4 : 0) + (m & S_IWUSR ? 2 : 0) + (m & S_IXUSR ? 1 : 0), \
    	(m & S_IRGRP ? 4 : 0) +	(m & S_IWGRP ? 2 : 0) +	(m & S_IXGRP ? 1 : 0), \
	(m & S_IROTH ? 4 : 0) +	(m & S_IWOTH ? 2 : 0) + (m & S_IXOTH ? 1 : 0)

/* Definition to shut gcc up about unused arguments. */
#define unused __attribute__ ((unused))

/* Attribute to make gcc check printf-like arguments. */
#define printflike1 __attribute__ ((format (printf, 1, 2)))
#define printflike2 __attribute__ ((format (printf, 2, 3)))
#define printflike3 __attribute__ ((format (printf, 3, 4)))
#define printflike4 __attribute__ ((format (printf, 4, 5)))

/* Ensure buffer size. */
#define ENSURE_SIZE(buf, len, size) do {				\
	(buf) = ensure_size(buf, &(len), 1, size);			\
} while (0)
#define ENSURE_SIZE2(buf, len, nmemb, size) do {			\
	(buf) = ensure_size(buf, &(len), nmemb, size);			\
} while (0)
#define ENSURE_FOR(buf, len, size, adj) do {				\
	(buf) = ensure_for(buf, &(len), size, adj);			\
} while (0)

/* Description buffer size. */
#define DESCBUFSIZE 512

/* Replace buffer size. */
#define REPLBUFSIZE 64

/* Lengths of time. */
#define TIME_MINUTE 60LL
#define TIME_HOUR 3600LL
#define TIME_DAY 86400LL
#define TIME_WEEK 604800LL
#define TIME_MONTH 2419200LL
#define TIME_YEAR 29030400LL

/* Valid email address chars. */
#define isaddr(c) ( 							\
	((c) >= 'a' && (c) <= 'z') || 					\
	((c) >= 'A' && (c) <= 'Z') ||					\
	((c) >= '0' && (c) <= '9') ||					\
	(c) == '&' || (c) == '*' || (c) == '+' || (c) == '?' ||	 	\
	(c) == '-' || (c) == '.' || (c) == '=' || (c) == '/' ||		\
	(c) == '^' || (c) == '{' || (c) == '}' || (c) == '~' || 	\
	(c) == '_' || (c) == '@' || (c) == '\'')

/* Number of matches to use. */
#define NPMATCH 10

/* Account name match. */
#define name_match(p, n) (fnmatch(p, n, 0) == 0)

/* Macros in configuration file. */
struct macro {
	char			 name[MAXNAMESIZE];
	union {
		long long	 num;
		char		*str;
	} value;
	enum {
		MACRO_NUMBER,
		MACRO_STRING
	} type;
	int			 fixed;

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
enum fdmop {
	FDMOP_NONE = 0,
	FDMOP_POLL,
	FDMOP_FETCH
};

/*
 * Wrapper struct for a string that needs tag replacement before it is used.
 * This is used for anything that needs to be replaced after account and mail
 * data are available, everything else is replaced at parse time.
 */
struct replstr {
	char		*str;
} __packed;
ARRAY_DECL(replstrs, struct replstr);

/* Similar to replstr but needs expand_path too. */
struct replpath {
	char		*str;
} __packed;

/* Server description. */
struct server {
	char		*host;
	char		*port;
	struct addrinfo	*ai;
	int		 ssl;
	int		 verify;
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

/* Shared memory. */
struct shm {
#ifdef SHM_SYSV
	key_t	 key;
	int	 id;
#define SHM_REGISTER(shm)
#define SHM_DEREGISTER(shm)
#endif
#ifdef SHM_MMAP
	char	 name[MAXPATHLEN];
	int	 fd;
#define SHM_REGISTER(shm) cleanup_register((shm)->name)
#define SHM_DEREGISTER(shm) cleanup_deregister((shm)->name)
#endif

	void	*data;
	size_t	 size;
};

/* Generic array of strings. */
ARRAY_DECL(strings, char *);

/* Options for final mail handling. */
enum decision {
	DECISION_NONE,
	DECISION_DROP,
	DECISION_KEEP
};

/* String block entry. */
struct strbent {
	size_t	key;
	size_t	value;
};

/* String block header. */
struct strb {
	u_int		 ent_used;
	u_int		 ent_max;

	size_t		 str_used;
	size_t	 	 str_size;
};

/* Initial string block slots and block size. */
#define STRBENTRIES 64
#define STRBBLOCK 1024

/* String block access macros. */
#define STRB_KEY(sb, sbe) (((char *) (sb)) + (sizeof *(sb)) + (sbe)->key)
#define STRB_VALUE(sb, sbe) (((char *) (sb)) + (sizeof *(sb)) + (sbe)->value)

#define STRB_ENTRY(sb, n) ((void *) (((char *) (sb)) + \
	(sizeof *(sb)) + (sb)->str_size + ((n) * (sizeof (struct strbent)))))
#define STRB_ENTSIZE(sb) ((sb)->ent_max * (sizeof (struct strbent)))
#define STRB_SIZE(sb) ((sizeof *(sb)) + (sb)->str_size + STRB_ENTSIZE((sb)))

/* Regexp wrapper structs. */
struct re {
	char		*str;
#ifndef PCRE
	regex_t		 re;
#else
	pcre		*pcre;
#endif
	int		 flags;
};

struct rm {
	int		 valid;

	size_t		 so;
	size_t		 eo;
};

struct rmlist {
	int		 valid;

	struct rm	 list[NPMATCH];
};

/* Regexp flags. */
#define RE_IGNCASE 0x1
#define RE_NOSUBST 0x2

/* A single mail. */
struct mail {
	u_int			 idx;
	double			 tim;

	struct strb		*tags;

	struct shm		 shm;

	struct attach		*attach;
	int			 attach_built;

	char			*base;

	char			*data;
	size_t			 off;

	size_t	 	 	 size;		/* size of mail */
	size_t	 	 	 space;		/* size of malloc'd area */

	ARRAY_DECL(, size_t)	 wrapped;	/* list of wrapped lines */
	char			 wrapchar;	/* wrapped character */

	ssize_t		 	 body;		/* offset of body */

	/* XXX move below into special struct and just cp it in mail_*? */
	struct rmlist		 rml;		/* regexp matches */

	enum decision		 decision;	/* final deliver decision */

	void			 (*auxfree)(void *);
	void			*auxdata;
};

/* Mail fetch/delivery return codes. */
#define MAIL_CONTINUE 0
#define MAIL_DELIVER 1
#define MAIL_MATCH 2
#define MAIL_ERROR 3
#define MAIL_BLOCKED 4
#define MAIL_DONE 5

/* Mail fetch/delivery context. */
struct mail_ctx {
	int				 done;
	u_int				 msgid;

	struct account			*account;
	struct io			*io;
	struct mail			*mail;

	struct rule			*rule;
	ARRAY_DECL(, struct rule *)	 stack;
	struct expritem			*expritem;
	int				 result;
	int				 matched;

	TAILQ_HEAD(, deliver_ctx)	 dqueue;

	TAILQ_ENTRY(mail_ctx)		 entry;
};
TAILQ_HEAD(mail_queue, mail_ctx);

/* An attachment. */
struct attach {
	u_int	 	 	 idx;

	size_t		 	 data;
	size_t	 	 	 body;
	size_t   	 	 size;

	char			*type;
	char			*name;

	struct attach		*parent;
	TAILQ_HEAD(, attach)	 children;

	TAILQ_ENTRY(attach)	 entry;
};

/* Privsep message types. */
enum msgtype {
	MSG_ACTION,
	MSG_EXIT,
	MSG_DONE,
	MSG_COMMAND
};

/* Privsep message data. */
struct msgdata {
	int	 		 	 error;
	struct mail		 	 mail;

	/* these only work so long as they aren't moved in either process */
	struct account			*account;
	struct actitem			*actitem;
	struct match_command_data	*cmddata;

	uid_t			 	 uid;
};

/* Privsep message buffer. */
struct msgbuf {
	void		*buf;
	size_t		 len;
};

/* Privsep message. */
struct msg {
	u_int		 id;
	enum msgtype	 type;
	size_t		 size;

	struct msgdata	 data;
};

/* A single child. */
struct child {
	pid_t		 pid;
	struct io	*io;

	void		*data;
	int		 (*msg)(struct child *, struct msg *, struct msgbuf *);

	void		*buf;
	size_t		 len;
};

/* List of children. */
ARRAY_DECL(children, struct child *);

/* Fetch child data. */
struct child_fetch_data {
	struct account	*account;
	enum fdmop	 op;
	struct children	*children;
};

/* Deliver child data. */
struct child_deliver_data {
	void			 (*hook)(int, struct account *, struct msg *,
				      struct child_deliver_data *, int *);

	struct child 		*child; /* the source of the request */

	u_int			 msgid;
	const char		*name;

	struct account		*account;
	struct mail		*mail;
	struct actitem		*actitem;

	struct deliver_ctx	*dctx;
	struct mail_ctx		*mctx;

	struct match_command_data *cmddata;
};

/* Users list. */
ARRAY_DECL(users, uid_t);

/* Account entry. */
struct account {
	u_int			 idx;

	char			 name[MAXNAMESIZE];

	struct users		*users;
	int			 find_uid;

	int			 disabled;
	int			 keep;

	struct fetch		*fetch;
	void			*data;

	TAILQ_ENTRY(account)	 entry;
};

/* Action item. */
struct actitem {
	u_int			 idx;

	struct deliver		*deliver;
	void			*data;

	TAILQ_ENTRY(actitem)	 entry;
};

/* Action list. */
TAILQ_HEAD(actlist, actitem);

/* Action definition. */
struct action {
	char			 name[MAXNAMESIZE];

	struct users		*users;
	int			 find_uid;

	struct actlist		*list;

	TAILQ_ENTRY(action)	 entry;
};

/* Actions arrays. */
ARRAY_DECL(actions, struct action *);

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

/* Expression struct. */
TAILQ_HEAD(expr, expritem);

/* Rule list. */
TAILQ_HEAD(rules, rule);

/* Rule entry. */
struct rule {
	u_int			 idx;

	struct strings		*accounts;
	struct expr		*expr;

	struct users		*users;
	int			 find_uid;	/* find uids from headers */

	int			 stop;		/* stop matching at this rule */

	struct rules		 rules;
	struct action		*lambda;
	struct replstrs		*actions;

	TAILQ_ENTRY(rule)	 entry;
};

/* Lock types. */
#define LOCK_FCNTL 0x1
#define LOCK_FLOCK 0x2
#define LOCK_DOTLOCK 0x4

/* Configuration settings. */
struct conf {
	int 			 debug;
	int			 syslog;

	uid_t			 child_uid;
	gid_t			 child_gid;
	char			*tmp_dir;

	struct strings	 	 incl;
	struct strings		 excl;

	struct proxy		*proxy;

	struct strings		*domains; /* domains to look for with users */
	struct strings		*headers; /* headers to search for users */

	struct {
		int		 valid;
		uid_t		 last_uid;

		char		*home;
		char		*user;
		char		*uid;
		char		*host;
		char		*fqdn;
		char		*addr;
	} info;

	char			*conf_file;
	char			*lock_file;
	int			 check_only;
	int			 allow_many;
	int			 keep_all;
	int			 no_received;
	int			 verify_certs;
	u_int			 purge_after;
	enum decision		 impl_act;

	int			 queue_high;
	int			 queue_low;

	mode_t			 file_umask;
	gid_t			 file_group;

	size_t			 max_size;
	int			 timeout;
	int		         del_big;
	u_int			 lock_types;
	uid_t			 def_user;

	TAILQ_HEAD(, account)	 accounts;
 	TAILQ_HEAD(, action)	 actions;
	struct rules		 rules;
};
extern struct conf		 conf;

/* Buffer structure. */
struct buffer {
	u_char		*base;		/* buffer start */
	size_t		 allocated;	/* total size of buffer */

	size_t		 size;		/* size of data in buffer */
	size_t		 offset;	/* offset of data in buffer */
};

/* Limits at which to fail. */
#define IO_MAXLINELEN (1024 * 1024) 		/* 1 MB */

/* IO line endings. */
#define IO_CRLF "\r\n"
#define IO_CR   "\r"
#define IO_LF   "\n"

/* Amount to attempt to append to the buffer each time. */
#define IO_BLOCKSIZE 16384

/* Initial line buffer length. */
#define IO_LINESIZE 256

/* Amount to poll after in io_update. */
#define IO_FLUSHSIZE (2 * IO_BLOCKSIZE)

/* Maximum number of pollfds. */
#define IO_POLLFDS 256

/* IO macros. */
#define IO_ROUND(n) (((n / IO_BLOCKSIZE) + 1) * IO_BLOCKSIZE)
#define IO_CLOSED(io) ((io)->flags & IOF_CLOSED)
#define IO_RDSIZE(io) (buffer_used((io)->rd))
#define IO_WRSIZE(io) (buffer_used((io)->wr))

/* IO structure. */
struct io {
	int		 fd;
	int		 dup_fd;	/* dup all data to this fd */
	SSL		*ssl;

	char		*error;

	int		 flags;
#define IOF_NEEDFILL 0x1
#define IOF_NEEDPUSH 0x2
#define IOF_CLOSED 0x4

	struct buffer	*rd;
	struct buffer	*wr;

	char		*lbuf;		/* line buffer */
	size_t		 llen;		/* line buffer size */

	int		 timeout;
	const char	*eol;
};

/* Command flags. */
#define CMD_IN  0x1
#define CMD_OUT 0x2
#define CMD_ONCE 0x4

/* Command data. */
struct cmd {
	pid_t	 	 pid;
	int		 status;
	int		 flags;
	int		 timeout;

	struct io	*io_in;
	struct io	*io_out;
	struct io	*io_err;
};

/* Comparison operators. */
enum cmp {
	CMP_EQ,
	CMP_NE,
	CMP_LT,
	CMP_GT
};

#ifdef NO_SETRESUID
#define setresuid(r, e, s) setreuid(r, e)
#endif

#ifdef NO_SETRESGID
#define setresgid(r, e, s) setregid(r, e)
#endif

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

#ifdef NO_ASPRINTF
/* asprintf.c */
int 			 asprintf(char **, const char *, ...);
int			 vasprintf(char **, const char *, va_list);
#endif

/* shm.c */
void			*shm_create(struct shm *, size_t);
int			 shm_owner(struct shm *, uid_t, gid_t);
void			 shm_destroy(struct shm *);
void			 shm_close(struct shm *);
void			*shm_reopen(struct shm *);
void			*shm_resize(struct shm *, size_t, size_t);

/* lex.l */
extern char		*curfile;
void			 include_start(char *);
int			 include_finish(void);

/* parse.y */
extern struct strb	*parse_tags;
extern struct macros	 macros;
struct users		*weed_users(struct users *);
struct strings 		*weed_strings(struct strings *);
void			 free_replstrs(struct replstrs *);
char 			*fmt_replstrs(const char *, struct replstrs *);
void			 free_strings(struct strings *);
char 			*fmt_strings(const char *, struct strings *);
char 			*fmt_users(const char *, struct users *);
struct macro		*find_macro(char *);
struct actions		*match_actions(char *);
void			 free_action(struct action *);
void			 free_rule(struct rule *);
void			 free_account(struct account *);
char			*expand_path(char *);

/* netrc.c */
FILE 			*netrc_open(const char *, char **);
void			 netrc_close(FILE *);
int			 netrc_lookup(FILE *, const char *, char **, char **);

/* fdm.c */
double			 get_time(void);
int			 dropto(uid_t);
int			 check_incl(char *);
int		         check_excl(char *);
int			 use_account(struct account *, char **);
void			 fill_info(const char *);
void			 fill_fqdn(char *, char **, char **);

/* re.c */
int			 re_compile(struct re *, char *, int, char **);
int			 re_string(struct re *, char *, struct rmlist *,
			     char **);
int			 re_block(struct re *, void *, size_t, struct rmlist *,
			     char **);
void			 re_free(struct re *);

/* attach.c */
struct attach 		*attach_visit(struct attach *, u_int *);
void printflike2	 attach_log(struct attach *, const char *, ...);
struct attach 		*attach_build(struct mail *);
void			 attach_free(struct attach *);

/* privsep.c */
int			 privsep_send(struct io *, struct msg *,
			     struct msgbuf *);
int			 privsep_check(struct io *);
int			 privsep_recv(struct io *, struct msg *,
			     struct msgbuf *);

/* command.c */
struct cmd 		*cmd_start(const char *, int, int, char *, size_t,
			     char **);
int			 cmd_poll(struct cmd *, char **, char **, char **,
			     size_t *, char **);
void			 cmd_free(struct cmd *);

/* child.c */
int			 child_fork(void);
__dead void		 child_exit(int);
struct child 		*child_start(struct children *, uid_t,
    			     int (*)(struct child *, struct io *),
			     int (*)(struct child *, struct msg *,
    			     struct msgbuf *), void *);

/* child-fetch.c */
int			 child_fetch(struct child *, struct io *);

/* child-deliver.c */
int			 child_deliver(struct child *, struct io *);
void			 child_deliver_action_hook(int, struct account *,
			     struct msg *, struct child_deliver_data *, int *);
void			 child_deliver_cmd_hook(int, struct account *,
			     struct msg *, struct child_deliver_data *, int *);

/* parent-fetch.c */
int			 parent_fetch(struct child *, struct msg *,
			     struct msgbuf *);

/* parent-deliver.c */
int			 parent_deliver(struct child *, struct msg *,
			     struct msgbuf *);

/* connect.c */
char 			*sslerror(const char *);
char 			*sslerror2(int, const char *);
struct proxy 		*getproxy(const char *);
struct io 		*connectproxy(struct server *, int, struct proxy *,
			     const char *, int, char **);
struct io		*connectio(struct server *, int, const char *, int,
			     char **);

/* mail.c */
int			 mail_open(struct mail *, size_t);
void			 mail_send(struct mail *, struct msg *);
int			 mail_receive(struct mail *, struct msg *, int);
void			 mail_close(struct mail *);
void			 mail_destroy(struct mail *);
int			 mail_resize(struct mail *, size_t);
char 			*rfc822_time(time_t, char *, size_t);
int			 openlock(const char *, u_int, int, mode_t);
void			 closelock(int, const char *, u_int);
int			 checkperms(const char *, const char *, int *);
void			 line_init(struct mail *, char **, size_t *);
void			 line_next(struct mail *, char **, size_t *);
int printflike3		 insert_header(struct mail *, const char *,
			     const char *, ...);
int			 remove_header(struct mail *, const char *);
char 			*find_header(struct mail *, const char *, size_t *,
			     int);
struct users		*find_users(struct mail *);
char			*find_address(char *, size_t, size_t *);
void			 trim_from(struct mail *);
char 		        *make_from(struct mail *);
u_int			 fill_wrapped(struct mail *);
void			 set_wrapped(struct mail *, char);

/* mail-state.c */
int	mail_match(struct mail_ctx *, struct msg *, struct msgbuf *);
int	mail_deliver(struct mail_ctx *, struct msg *, struct msgbuf *);

/* cleanup.c */
void			 cleanup_check(void);
void			 cleanup_flush(void);
void			 cleanup_purge(void);
void			 cleanup_register(char *);
void			 cleanup_deregister(char *);

/* strb.c */
void		 	 strb_create(struct strb **);
void			 strb_clear(struct strb **);
void			 strb_destroy(struct strb **);
void			 strb_dump(struct strb *, const char *,
    			     void (*)(const char *, ...));
void printflike3	 strb_add(struct strb **, const char *, const char *,
			     ...);
void			 strb_vadd(struct strb **, const char *, const char *,
			     va_list);
struct strbent 		*strb_find(struct strb *, const char *);
struct strbent	 	*strb_match(struct strb *, const char *);

/* replace.c */
void printflike3	 add_tag(struct strb **, const char *, const char *,
			     ...);
const char 		*find_tag(struct strb *, const char *);
const char		*match_tag(struct strb *, const char *);
void			 default_tags(struct strb **, char *, struct account *);
void			 update_tags(struct strb **);
char 			*replacestr(struct replstr *, struct strb *,
			     struct mail *, struct rmlist *);
char 			*replacepath(struct replpath *, struct strb *,
    			     struct mail *, struct rmlist *);

/* buffer.c */
struct buffer 		*buffer_create(size_t);
void			 buffer_destroy(struct buffer *);
size_t			 buffer_used(struct buffer *);
size_t			 buffer_free(struct buffer *);
u_char 			*buffer_start(struct buffer *);
u_char 			*buffer_end(struct buffer *);
void			 buffer_clear(struct buffer *);
void			 buffer_ensure(struct buffer *, size_t);
void			 buffer_added(struct buffer *, size_t);
void			 buffer_removed(struct buffer *, size_t);
void			 buffer_copyin(struct buffer *, const void *, size_t);
void			 buffer_copyout(struct buffer *, void *, size_t);

/* io.c */
struct io		*io_create(int, SSL *, const char *, int);
void			 io_readonly(struct io *);
void			 io_writeonly(struct io *);
void			 io_free(struct io *);
void			 io_close(struct io *);
int			 io_polln(struct io **, u_int, struct io **, int,
			     char **);
int			 io_poll(struct io *, char **);
int			 io_read2(struct io *, void *, size_t);
void 			*io_read(struct io *, size_t);
void			 io_write(struct io *, const void *, size_t);
char 			*io_readline2(struct io *, char **, size_t *);
char 			*io_readline(struct io *);
void printflike2	 io_writeline(struct io *, const char *, ...);
void			 io_vwriteline(struct io *, const char *, va_list);
int			 io_pollline2(struct io *, char **, char **, size_t *,
			     char **);
int			 io_pollline(struct io *, char **, char **);
int			 io_flush(struct io *, char **);
int			 io_wait(struct io *, size_t, char **);
int			 io_update(struct io *, char **);

/* log.c */
void			 vlog(FILE *, int, const char *, va_list);
void			 log_init(int);
void			 log_syslog(int);
void printflike1	 log_warn(const char *, ...);
void printflike1	 log_warnx(const char *, ...);
void printflike1	 log_info(const char *, ...);
void printflike1	 log_debug(const char *, ...);
void printflike1	 log_debug2(const char *, ...);
void printflike1	 log_debug3(const char *, ...);
__dead void		 fatal(const char *);
__dead void		 fatalx(const char *);

/* xmalloc.c */
void		*ensure_size(void *, size_t *, size_t, size_t);
void		*ensure_for(void *, size_t *, size_t, size_t);
char		*xstrdup(const char *);
void		*xxcalloc(size_t, size_t);
void		*xxmalloc(size_t);
void		*xxrealloc(void *, size_t, size_t);
void		 xxfree(void *);
int printflike2	 xxasprintf(char **, const char *, ...);
int		 xxvasprintf(char **, const char *, va_list);
int printflike3	 xsnprintf(char *, size_t, const char *, ...);
int		 xvsnprintf(char *, size_t, const char *, va_list);
int printflike3	 printpath(char *, size_t, const char *, ...);
char 		*xdirname(const char *);
char 		*xbasename(const char *);

/* xmalloc-debug.c */
#ifdef DEBUG
void		 xmalloc_callreport(const char *);

void		 xmalloc_clear(void);
void		 xmalloc_report(const char *);

void		*dxmalloc(const char *, u_int, size_t);
void		*dxcalloc(const char *, u_int, size_t, size_t);
void		*dxrealloc(const char *, u_int, void *, size_t, size_t);
void		 dxfree(const char *, u_int, void *);
int printflike4	 dxasprintf(const char *, u_int, char **, const char *, ...);
int		 dxvasprintf(const char *, u_int, char **, const char *,
		     va_list);
#endif

#ifdef DEBUG
#define xmalloc(s) dxmalloc(__FILE__, __LINE__, s)
#define xcalloc(n, s) dxcalloc(__FILE__, __LINE__, n, s)
#define xrealloc(p, n, s) dxrealloc(__FILE__, __LINE__, p, n, s)
#define xfree(p) dxfree(__FILE__, __LINE__, p)
#define xasprintf(pp, ...) dxasprintf(__FILE__, __LINE__, pp, __VA_ARGS__)
#define xvasprintf(pp, fmt, ap) dxvasprintf(__FILE__, __LINE__, pp, fmt, ap)
#else
#define xmalloc(s) xxmalloc(s)
#define xcalloc(n, s) xxcalloc(n, s)
#define xrealloc(p, n, s) xxrealloc(p, n, s)
#define xfree(p) xxfree(p)
#define xasprintf(pp, ...) xxasprintf(pp, __VA_ARGS__)
#define xvasprintf(pp, fmt, ap) xxvasprintf(pp, fmt, ap)
#endif

#endif /* FDM_H */
