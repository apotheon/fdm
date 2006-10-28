/* $Id: parse.y,v 1.52 2006-10-28 10:52:50 nicm Exp $ */

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

/* Declarations */

%{
#include <sys/types.h>
#include <sys/socket.h>

#include <ctype.h>
#include <fnmatch.h>
#include <libgen.h>
#include <limits.h>
#include <netdb.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include "fdm.h"

struct macros			 macros = TAILQ_HEAD_INITIALIZER(macros);

int			 	 rules;
struct saved {
	FILE			*yyin;
	int		 	 yylineno;
	char			*curfile;
};
ARRAY_DECLARE(, struct saved *)	 stack;
char				*curfile;

extern FILE			*yyin;
extern int		 	 yylineno;
extern int 		 	 yylex(void);
extern void		 	 yyrestart(FILE *);

int			 	 yyparse(void);
__dead printflike1 void  	 yyerror(const char *, ...);
int 			 	 yywrap(void);

struct account 			*find_account(char *);
struct action  			*find_action(char *);
struct macro			*find_macro(char *);

__dead printflike1 void
yyerror(const char *fmt, ...)
{
	va_list	 ap;
	char	*s, *file;

	file = curfile == NULL ? conf.conf_file : curfile;
	xasprintf(&s, "%s: %s at line %d", file, fmt, yylineno);

	va_start(ap, fmt);
	vlog(LOG_CRIT, s, ap);
	va_end(ap);

	exit(1);
}

int
yywrap(void)
{
	struct macro	*macro;
	struct saved	*old;
	char		*file;

	file = curfile == NULL ? conf.conf_file : curfile;
	log_debug2("finished file %s", file);

	if (ARRAY_EMPTY(&stack)) {
		while (!TAILQ_EMPTY(&macros)) {
			macro = TAILQ_FIRST(&macros);
			TAILQ_REMOVE(&macros, macro, entry);
			if (macro->type == MACRO_STRING)
				xfree(macro->value.string);
			xfree(macro);
		}

		ARRAY_FREE(&stack);
		return (1);
	}
	
	old = ARRAY_ITEM(&stack, ARRAY_LENGTH(&stack) - 1, struct saved *);
	yyin = old->yyin;
	yyrestart(yyin);
	yylineno = old->yylineno;
	xfree(curfile);
	curfile = old->curfile;
	xfree(old);
	ARRAY_REMOVE(&stack, ARRAY_LENGTH(&stack) - 1, struct saved *);

        return (0);
}

struct account *
find_account(char *name)
{
	struct account	*a;

	TAILQ_FOREACH(a, &conf.accounts, entry) {
		if (name_match(name, a->name))
			return (a);
	}
	return (NULL);
}

struct action *
find_action(char *name)
{
	struct action	*t;

	TAILQ_FOREACH(t, &conf.actions, entry) {
		if (strcmp(t->name, name) == 0)
			return (t);
	}
	return (NULL);
}

struct macro *
find_macro(char *name)
{
	struct macro	*macro;

	TAILQ_FOREACH(macro, &macros, entry) {
		if (strcmp(macro->name, name) == 0)
			return (macro);
	}

	return (NULL);
}
%}

%token TOKALL TOKACCOUNT TOKSERVER TOKPORT TOKUSER TOKPASS TOKACTION
%token TOKSET TOKACCOUNTS TOKMATCH TOKIN TOKCONTINUE TOKSTDIN TOKPOP3 TOKPOP3S
%token TOKNONE TOKCASE TOKAND TOKOR TOKTO TOKACTIONS TOKHEADERS TOKBODY
%token TOKMAXSIZE TOKDELTOOBIG TOKLOCKTYPES TOKDEFUSER TOKDOMAIN TOKDOMAINS
%token TOKHEADER TOKFROMHEADERS TOKUSERS TOKMATCHED TOKUNMATCHED TOKNOT
%token TOKIMAP TOKIMAPS TOKDISABLED TOKFOLDER TOKPROXY TOKALLOWMANY TOKINCLUDE
%token TOKLOCKFILE
%token ACTPIPE ACTSMTP ACTDROP ACTMAILDIR ACTMBOX ACTWRITE ACTAPPEND ACTREWRITE
%token LCKFLOCK LCKFCNTL LCKDOTLOCK

%union
{
	long long 	 	 number;
        char 			*string;
	int 		 	 flag;
	u_int			 locks;
	struct {
		struct fetch	*fetch;
		void		*data;
	} fetch;
	struct {
		char		*host;
		char		*port;
	} server;
	enum area	 	 area;
	enum op			 op;
	struct accounts		*accounts;
	struct action	 	 action;
	struct actions		*actions;
	struct domains		*domains;
	struct headers	 	*headers;
	struct match		*match;
	struct {
		struct matches	*matches;
		int		 type;
	} matches;
	uid_t			 uid;
	struct {
		struct users	*users;
		int		 find_uid;
	} users;
}

%token <number> NUMBER SIZE
%token <string> STRING STRMACRO STRMACROB NUMMACRO NUMMACROB

%type  <accounts> accounts accountslist
%type  <action> action
%type  <actions> actions actionslist
%type  <area> area
%type  <domains> domains domainslist
%type  <fetch> fetchtype
%type  <flag> cont icase not disabled poptype imaptype
%type  <headers> headers headerslist
%type  <locks> lock locklist
%type  <match> match
%type  <matches> matches matchlist
%type  <number> size num
%type  <op> op
%type  <server> server
%type  <string> port to folder str
%type  <uid> uid
%type  <users> users userslist

%%

/* Rules */

cmds: /* empty */
    | cmds account
    | cmds defaction
    | cmds defmacro
    | cmds include
    | cmds rule
    | cmds set

str: STRING
     {
	     $$ = $1;
     }
   | STRMACRO
     {
	     struct macro	*macro;

	     if (strlen($1) > MAXNAMESIZE)
		     yyerror("macro name too long: %s", $1);

	     if ((macro = find_macro($1)) == NULL)
		     yyerror("undefined macro: %s", $1);
	     if (macro->type != MACRO_STRING)
		     yyerror("string macro expected: %s", $1);

	     $$ = xstrdup(macro->value.string);

	     xfree($1);
     }
   | STRMACROB
     {
	     struct macro	*macro;
	     char 		 name[MAXNAMESIZE];

	     if (strlen($1) > MAXNAMESIZE)
 		     yyerror("macro name too long: %s", $1);

	     name[0] = $1[0];
	     name[1] = '\0';
	     strlcat(name, $1 + 2, MAXNAMESIZE);
	     name[strlen(name) - 1] = '\0';

	     if ((macro = find_macro(name)) == NULL)
		     yyerror("undefined macro: %s", name);
	     if (macro->type != MACRO_STRING)
		     yyerror("string macro expected: %s", name);

	     $$ = xstrdup(macro->value.string);

	     xfree($1);
     }

num: NUMBER
     {
	     $$ = $1;
     }
   | NUMMACRO
     {
	     struct macro	*macro;

	     if (strlen($1) > MAXNAMESIZE)
		     yyerror("macro name too long: %s", $1);

	     if ((macro = find_macro($1)) == NULL)
		     yyerror("undefined macro: %s", $1);
	     if (macro->type != MACRO_NUMBER)
		     yyerror("number macro expected: %s", $1);

	     $$ = macro->value.number;

	     xfree($1);
     }
   | NUMMACROB
     {
	     struct macro	*macro;
	     char 		 name[MAXNAMESIZE];

	     if (strlen($1) > MAXNAMESIZE)
		     yyerror("macro name too long: %s", $1);

	     name[0] = $1[0];
	     name[1] = '\0';
	     strlcat(name, $1 + 2, MAXNAMESIZE);
	     name[strlen(name) - 1] = '\0';

	     if ((macro = find_macro(name)) == NULL)
		     yyerror("undefined macro: %s", name);
	     if (macro->type != MACRO_NUMBER)
		     yyerror("number macro expected: %s", name);

	     $$ = macro->value.number;

	     xfree($1);
     }

include: TOKINCLUDE str
	 {
		 char		*path;
		 struct saved	*old;

		 if (*$2 == '\0')
			 yyerror("invalid include file");
		 
		 old = xmalloc(sizeof *old);
		 old->yyin = yyin;
		 old->yylineno = yylineno;
		 old->curfile = curfile;
		 ARRAY_ADD(&stack, old, struct saved *);

		 yyin = fopen($2, "r");
		 if (yyin == NULL) {
			 xasprintf(&path, "%s/%s", dirname(conf.conf_file), $2);
			 if (access(path, R_OK) != 0)
				 yyerror("%s: %s", $2, strerror(errno));
			 yyin = fopen(path, "r");
			 if (yyin == NULL)
				 yyerror("%s: %s", path, strerror(errno));
			 curfile = path;
			 xfree($2);
		 } else
			 curfile = $2;
		 log_debug2("including file %s", curfile);
		 yyrestart(yyin);
		 yylineno = 0;
	 }

size: num
    | SIZE
      {
	      $$ = $1;
      }

set: TOKSET TOKMAXSIZE size
     {
	     if ($3 > MAXMAILSIZE)
		     yyerror("maximum size too large: %lld", $3);
	     conf.max_size = $3;
     }
   | TOKSET TOKLOCKTYPES locklist
     {
	     if ($3 & LOCK_FCNTL && $3 & LOCK_FLOCK)
		     yyerror("fcntl and flock locking cannot be used together");
	     conf.lock_types = $3;
     }
   | TOKSET TOKLOCKFILE str
     {
	     if (*$3 == '\0')
		     yyerror("invalid lock file");

	     if (conf.lock_file != NULL)
		     xfree(conf.lock_file);
	     conf.lock_file = $3;
     }
   | TOKSET TOKDELTOOBIG
     {
	     conf.del_big = 1;
     }
   | TOKSET TOKALLOWMANY
     {
	     conf.allow_many = 1;
     }
   | TOKSET TOKDEFUSER uid
     {
	     conf.def_user = $3;
     }
   | TOKSET domains
     {
	     u_int	i;

	     if (conf.domains != NULL) {
		     for (i = 0; i < ARRAY_LENGTH(conf.domains); i++)
			     xfree(ARRAY_ITEM(conf.domains, i, void *));
		     ARRAY_FREE(conf.domains);
		     xfree(conf.domains);
	     }
		     
	     conf.domains = $2;
     }
   | TOKSET headers
     {
	     u_int	i;

	     if (conf.headers != NULL) {
		     for (i = 0; i < ARRAY_LENGTH(conf.headers); i++)
			     xfree(ARRAY_ITEM(conf.headers, i, void *));
		     ARRAY_FREE(conf.headers);
		     xfree(conf.headers);
	     }

	     conf.headers = $2;
     }
   | TOKSET TOKPROXY str
     {
	     if (conf.proxy != NULL) {
		     xfree(conf.proxy->server.host);
		     xfree(conf.proxy->server.port);
		     if (conf.proxy->user != NULL)
			     xfree(conf.proxy->user);
		     if (conf.proxy->pass != NULL)
			     xfree(conf.proxy->pass);
	     }
	     if ((conf.proxy = getproxy($3)) == NULL)
		     yyerror("invalid proxy");
	     xfree($3);
     }

defmacro: STRMACRO '=' STRING
     	  {
		  struct macro	*macro;
		  
		  if ((macro = find_macro($1)) == NULL) {
			  macro = xmalloc(sizeof *macro);
			  if (strlen($1) > MAXNAMESIZE)
				  yyerror("macro name too long: %s", $1);
			  strlcpy(macro->name, $1, sizeof macro->name);
			  TAILQ_INSERT_HEAD(&macros, macro, entry);
		  }
		  macro->type = MACRO_STRING;
		  macro->value.string = $3;
		  xfree($1);
	  }
        | NUMMACRO '=' NUMBER
	  {	
	     struct macro	*macro;
	     
	     if ((macro = find_macro($1)) == NULL) {
		     macro = xmalloc(sizeof *macro);
		     if (strlen($1) > MAXNAMESIZE)
			     yyerror("macro name too long: %s", $1);
		     strlcpy(macro->name, $1, sizeof macro->name);
		     TAILQ_INSERT_HEAD(&macros, macro, entry);
	     }
	     macro->type = MACRO_NUMBER;
	     macro->value.number = $3;
	     xfree($1);
	  }

domains: TOKDOMAIN str
	 {
		 char	*cp;

		 if (*$2 == '\0')
			 yyerror("invalid domain");

		 $$ = xmalloc(sizeof (struct domains));
		 ARRAY_INIT($$);
		 for (cp = $2; *cp != '\0'; cp++)
			 *cp = tolower((int) *cp);
		 ARRAY_ADD($$, $2, char *);
	 }
       | TOKDOMAINS '{' domainslist '}'
	 {
		 $$ = $3;
	 }

domainslist: domainslist str
	     {
		     char	*cp;

		     if (*$2 == '\0')
			     yyerror("invalid domain");

		     $$ = $1;
		     for (cp = $2; *cp != '\0'; cp++)
			     *cp = tolower((int) *cp);
		     ARRAY_ADD($$, $2, char *);
	     }
	   | str
	     {
		     char	*cp;

		     if (*$1 == '\0')
			     yyerror("invalid domain");

		     $$ = xmalloc(sizeof (struct domains));
		     ARRAY_INIT($$);
		     for (cp = $1; *cp != '\0'; cp++)
			     *cp = tolower((int) *cp);
		     ARRAY_ADD($$, $1, char *);
	     }

headers: TOKHEADER str
	 {
		 char	*cp;

		 if (*$2 == '\0')
			 yyerror("invalid header");

		 $$ = xmalloc(sizeof (struct headers));
		 ARRAY_INIT($$);
		 for (cp = $2; *cp != '\0'; cp++)
			 *cp = tolower((int) *cp);
		 ARRAY_ADD($$, $2, char *);
	 }
       | TOKHEADERS '{' headerslist '}'
	 {
		 $$ = $3;
	 }

headerslist: headerslist str
	     {
		     char	*cp;

		     if (*$2 == '\0')
			     yyerror("invalid header");

		     $$ = $1;
		     for (cp = $2; *cp != '\0'; cp++)
			     *cp = tolower((int) *cp);
		     ARRAY_ADD($$, $2, char *);
	     }
	   | str
	     {
		     char	*cp;

		     if (*$1 == '\0')
			     yyerror("invalid header");

		     $$ = xmalloc(sizeof (struct headers));
		     ARRAY_INIT($$);
		     for (cp = $1; *cp != '\0'; cp++)
			     *cp = tolower((int) *cp);
		     ARRAY_ADD($$, $1, char *);
	     }

lock: LCKFCNTL
      {
	      $$ |= LOCK_FCNTL;
      }
    | LCKFLOCK
      {
	      $$ |= LOCK_FLOCK;
      }
    | LCKDOTLOCK
      {
	      $$ |= LOCK_DOTLOCK;
      }

locklist: locklist lock
	  {
		  $$ = $1 | $2;
	  }
	| lock
	  {
		  $$ = $1;
	  }
	| TOKNONE
	  {
		  $$ = 0;
	  }

uid: str
     {
	     struct passwd	*pw;

	     pw = getpwnam($1);
	     if (pw == NULL)
		     yyerror("unknown user: %s", $1);
	     $$ = pw->pw_uid;
	     endpwent();

	     xfree($1);
     }
   | num
     {
	     struct passwd	*pw;

	     if ($1 > UID_MAX)
		     yyerror("invalid uid: %llu", $1);
	     pw = getpwuid($1);
	     if (pw == NULL)
		     yyerror("unknown uid: %llu", $1);
	     $$ = pw->pw_uid;
	     endpwent();
     }

users: /* empty */
       {
	       $$.users = NULL;
	       $$.find_uid = 0;
       }
     | TOKUSER TOKFROMHEADERS
       {
	       $$.users = NULL;
	       $$.find_uid = 1;
       }
     | TOKUSERS TOKFROMHEADERS
       {
	       $$.users = NULL;
	       $$.find_uid = 1;
       }
     | TOKUSER uid
       {
	       $$.users = xmalloc(sizeof (struct users));
	       ARRAY_INIT($$.users);
	       ARRAY_ADD($$.users, $2, uid_t);
	       $$.find_uid = 0;
       }
     | TOKUSERS '{' userslist '}'
       {
	       $$ = $3;
	       $$.find_uid = 0;
       }

userslist: userslist uid
	   {
		   $$ = $1;
		   ARRAY_ADD($$.users, $2, uid_t);
	   }
	 | uid
	   {
		   $$.users = xmalloc(sizeof (struct users));
		   ARRAY_INIT($$.users);
		   ARRAY_ADD($$.users, $1, uid_t);
	   }

icase: TOKCASE
      {
	      /* match case */
	      $$ = 0;
      }
    | /* empty */
      {
	      /* ignore case */
	      $$ = 1;
      }

not: TOKNOT
      {
	      $$ = 1;
      }
    | /* empty */
      {
	      $$ = 0;
      }

disabled: TOKDISABLED
          {
		  $$ = 1;
          }
        | /* empty */
	  {
		  $$ = 0;
	  }

port: TOKPORT str
      {
	      if (*$2 == '\0')
		      yyerror("invalid port");

	      $$ = $2;
      }
    | TOKPORT num
      {
	      xasprintf(&$$, "%lld", $2);
      }

server: TOKSERVER str port
	{
		if (*$2 == '\0')
			yyerror("invalid host");

		$$.host = $2;
		$$.port = $3;
	}
      | TOKSERVER str
	{
		if (*$2 == '\0')
			yyerror("invalid host");

		$$.host = $2;
		$$.port = NULL;
	}

to: /* empty */
    {
	    $$ = NULL;
    }
  | TOKTO str
    {
	    if (*$2 == '\0')
		    yyerror("invalid to");

	    $$ = $2;
    }

action: ACTPIPE str
	{
		if (*$2 == '\0')
			yyerror("invalid command");

		$$.deliver = &deliver_pipe;
		$$.data = $2;
	}
      | ACTREWRITE str
	{
		if (*$2 == '\0')
			yyerror("invalid command");

		$$.deliver = &deliver_rewrite;
		$$.data = $2;
	}
      | ACTWRITE str
	{
		if (*$2 == '\0')
			yyerror("invalid path");

		$$.deliver = &deliver_write;
		$$.data = $2;
	}
      | ACTAPPEND str
	{
		if (*$2 == '\0')
			yyerror("invalid path");

		$$.deliver = &deliver_append;
		$$.data = $2;
	}
      | ACTMAILDIR str
	{
		if (*$2 == '\0')
			yyerror("invalid path");

		$$.deliver = &deliver_maildir;
		$$.data = $2;
	}
      | ACTMBOX str
	{
		if (*$2 == '\0')
			yyerror("invalid path");

		$$.deliver = &deliver_mbox;
		$$.data = $2;
	}
      | ACTSMTP server to
	{
		struct smtp_data	*data;

		$$.deliver = &deliver_smtp;

		data = xcalloc(1, sizeof *data);
		$$.data = data;

		data->server.host = $2.host;
		data->server.port = $2.port != NULL ? $2.port : "smtp";
		data->server.ai = NULL;
		data->to = $3;
	}
      | ACTDROP
        {
		$$.deliver = &deliver_drop;
	}

defaction: TOKACTION str users action
	 {
		 struct action	*t;

		 if (strlen($2) >= MAXNAMESIZE)
			 yyerror("action name too long: %s", $2);
		 if (*$2 == '\0')
			 yyerror("invalid action name");
		 if (find_action($2) != NULL)
			 yyerror("duplicate action: %s", $2);
		 
		 t = xmalloc(sizeof *t);
		 memcpy(t, &$4, sizeof *t);
		 strlcpy(t->name, $2, sizeof t->name);
		 t->users = $3.users;
		 t->find_uid = $3.find_uid;
		 TAILQ_INSERT_TAIL(&conf.actions, t, entry);
		 
		 log_debug2("added action: name=%s deliver=%s", t->name,
		     t->deliver->name);
		 
		 xfree($2);
	 }

accounts: /* empty */
	  {
		  $$ = NULL;
	  }
        | TOKACCOUNT str
	  {
		  if (*$2 == '\0')
			  yyerror("invalid account name");

		  $$ = xmalloc(sizeof (struct accounts));
		  ARRAY_INIT($$);
		  if (find_account($2) == NULL)
			  yyerror("no matching accounts: %s", $2);
		  ARRAY_ADD($$, $2, char *);
	  }
	| TOKACCOUNTS '{' accountslist '}'
	  {
		  $$ = $3;
	  }

accountslist: accountslist str
 	      {
		      if (*$2 == '\0')
			      yyerror("invalid account name");

		      $$ = $1;
		      if (find_account($2) == NULL)
			      yyerror("no matching accounts: %s", $2);
		      ARRAY_ADD($$, $2, char *);
	      }
	    | str
	      {
		      if (*$1 == '\0')
			      yyerror("invalid account name");

		      $$ = xmalloc(sizeof (struct accounts));
		      ARRAY_INIT($$);
		      if (find_account($1) == NULL)
			      yyerror("no matching accounts: %s", $1);
		      ARRAY_ADD($$, $1, char *);
	      }

actions: TOKACTION str
	 {
		 struct action	*t;

		 if (*$2 == '\0')
			 yyerror("invalid action name");

		 $$ = xmalloc(sizeof (struct actions));
		 ARRAY_INIT($$);
		 if ((t = find_action($2)) == NULL)
			 yyerror("unknown action: %s", $2);
		 ARRAY_ADD($$, t, struct action *);
		 free($2);
	 }
       | TOKACTIONS '{' actionslist '}'
         {
		 $$ = $3;
	 }

actionslist: actionslist str
	     {
		     struct action	*t;

		     if (*$2 == '\0')
			     yyerror("invalid action name");

		     $$ = $1;
		     if ((t = find_action($2)) == NULL)
			     yyerror("unknown action: %s", $2);
		     ARRAY_ADD($$, t, struct action *);
		     free($2);
	     }
	   | str
	     {
		     struct action	*t;

		     if (*$1 == '\0')
			     yyerror("invalid action name");

		     $$ = xmalloc(sizeof (struct actions));
		     ARRAY_INIT($$);
		     if ((t = find_action($1)) == NULL)
			     yyerror("unknown action: %s", $1);
		     ARRAY_ADD($$, t, struct action *);
		     free($1);
	     }

cont: /* empty */
      {
	      $$ = 0;
      }
    | TOKCONTINUE
      {
	      $$ = 1;
      }

area: /* empty */
      {
	      $$ = AREA_ANY;
      }
    | TOKIN TOKALL
      {
	      $$ = AREA_ANY;
      }
    | TOKIN TOKHEADERS
      {
	      $$ = AREA_HEADERS;
      }
    | TOKIN TOKBODY
      {
	      $$ = AREA_BODY;
      }

op: TOKAND
    {
	    $$ = OP_AND;
    }
  | TOKOR
    {
	    $$ = OP_OR;
    }

match: not icase str area
       {
	       int	 error, flags;
	       size_t	 len;
	       char	*buf;

	       if (*$3 == '\0')
		       yyerror("invalid regexp");

	       $$ = xcalloc(1, sizeof (struct match));
	       $$->s = $3;
	       $$->op = OP_NONE;
	       $$->area = $4;
	       $$->inverted = $1;

	       flags = REG_EXTENDED|REG_NOSUB|REG_NEWLINE;
	       if ($2)
		       flags |= REG_ICASE;
	       if ((error = regcomp(&$$->re, $3, flags)) != 0) {
		       len = regerror(error, &$$->re, NULL, 0);
		       buf = xmalloc(len);
		       regerror(error, &$$->re, buf, len);
		       yyerror("%s", buf);
	       }
       }

matchlist: matchlist op match
	   {
		   $$ = $1;
		   $3->op = $2;
		   TAILQ_INSERT_TAIL($$.matches, $3, entry);
		   $$.type = RULE_MATCHES;
	   }
         | op match
	   {
		   $$.matches = xcalloc(1, sizeof (struct matches));
		   $2->op = $1;
		   TAILQ_INSERT_HEAD($$.matches, $2, entry);
		   $$.type = RULE_MATCHES;
	   }

matches: TOKMATCH match matchlist
         {
		 if ($3.matches != NULL)
			 $$ = $3;
		 else
			 $$.matches = xcalloc(1, sizeof (struct matches));
		 TAILQ_INSERT_HEAD($$.matches, $2, entry);
		 $$.type = RULE_MATCHES;
	 }
       | TOKMATCH match
         {
		 $$.matches = xcalloc(1, sizeof (struct matches));
		 TAILQ_INSERT_HEAD($$.matches, $2, entry);
		 $$.type = RULE_MATCHES;
	 }
       | TOKMATCH TOKALL
	 {
		 $$.matches = NULL;
		 $$.type = RULE_ALL;
	 }
       | TOKMATCH TOKMATCHED
	 {
		 $$.matches = NULL;
		 $$.type = RULE_MATCHED;
	 }
       | TOKMATCH TOKUNMATCHED
	 {
		 $$.matches = NULL;
		 $$.type = RULE_UNMATCHED;
	 }

rule: matches accounts users actions cont
      {
	      struct rule	*r;
	      struct match	*c;
	      char		 tmp[1024], tmp2[1024];
	      u_int		 i;

	      r = xcalloc(1, sizeof *r);
	      r->index = rules++;
	      r->stop = !$5;
	      r->accounts = $2;
	      r->matches = $1.matches;
	      r->type = $1.type;
	      r->users = $3.users;
	      r->find_uid = $3.find_uid;
	      r->actions = $4;

	      TAILQ_INSERT_TAIL(&conf.rules, r, entry);

	      if (r->matches == NULL)
		      xsnprintf(tmp, sizeof tmp, "all");
	      else {
		      *tmp = '\0';
		      TAILQ_FOREACH(c, r->matches, entry) {
			      switch (c->op) {
			      case OP_AND:
				      strlcat(tmp, "and \"", sizeof tmp);
				      break;
			      case OP_OR:
				      strlcat(tmp, "or \"", sizeof tmp);
				      break;
			      case OP_NONE:
				      strlcat(tmp, "\"", sizeof tmp);
				      break;
			      }
			      strlcat(tmp, c->s, sizeof tmp);
			      strlcat(tmp, "\" ", sizeof tmp);
			      switch (c->area) {
			      case AREA_BODY:
				      strlcat(tmp, "in body ", sizeof tmp);
				      break;
			      case AREA_HEADERS:
				      strlcat(tmp, "in headers ", sizeof tmp);
				      break;
			      case AREA_ANY:
				      strlcat(tmp, "in any ", sizeof tmp);
				      break;
			      }
		      }
	      }
	      *tmp2 = '\0';
	      for (i = 0; i < ARRAY_LENGTH($4); i++) {
		      strlcat(tmp2, ARRAY_ITEM($4, i, struct action *)->name,
			  sizeof tmp2);
		      strlcat(tmp2, " ", sizeof tmp2);
	      }

	      log_debug2("added rule: index=%u actions=%smatches=%s", r->index,
		  tmp2, tmp);
      }

folder: /* empty */
        {
		$$ = NULL;
        }
      | TOKFOLDER str
	{
		if (*$2 == '\0')
			yyerror("invalid folder");

		$$ = $2;
	}

poptype: TOKPOP3
         {
		 $$ = 0;
         }
       | TOKPOP3S
	 {
		 $$ = 1;
	 }

imaptype: TOKIMAP
          {
		  $$ = 0;
          }
        | TOKIMAPS
	  {
		  $$ = 1;
	  }

fetchtype: poptype server TOKUSER str TOKPASS str
           {
		   struct pop3_data	*data;

		   if (*$4 == '\0')
			   yyerror("invalid user");
		   if (*$6 == '\0')
			   yyerror("invalid pass");

		   $$.fetch = &fetch_pop3;
		   data = xcalloc(1, sizeof *data);
		   $$.data = data;
		   data->user = $4;
		   data->pass = $6;
		   data->server.ssl = $1;
		   data->server.host = $2.host;
		   data->server.port =
		       $2.port != NULL ? $2.port : $$.fetch->port;
		   data->server.ai = NULL;
	   }
         | imaptype server TOKUSER str TOKPASS str folder
           {
		   struct imap_data	*data;

		   if (*$4 == '\0')
			   yyerror("invalid user");
		   if (*$6 == '\0')
			   yyerror("invalid pass");

		   $$.fetch = &fetch_imap;
		   data = xcalloc(1, sizeof *data);
		   $$.data = data;
		   data->user = $4;
		   data->pass = $6;
		   data->folder = $7;
		   data->server.ssl = $1;
		   data->server.host = $2.host;
		   data->server.port =
		       $2.port != NULL ? $2.port : $$.fetch->port;
		   data->server.ai = NULL;
	   }
	 | TOKSTDIN
	   {
		   $$.fetch = &fetch_stdin;
		   $$.data = xmalloc(sizeof (struct stdin_data));
	   }

account: TOKACCOUNT str disabled fetchtype
         {
		 struct account		*a;

		 if (strlen($2) >= MAXNAMESIZE)
			 yyerror("account name too long: %s", $2);
		 if (*$2 == '\0')
			 yyerror("invalid account name");
		 if (find_account($2) != NULL)
			 yyerror("duplicate account: %s", $2);

		 a = xcalloc(1, sizeof *a);
		 strlcpy(a->name, $2, sizeof a->name);
		 a->disabled = $3;
		 a->fetch = $4.fetch;
		 a->data = $4.data;
		 TAILQ_INSERT_TAIL(&conf.accounts, a, entry);

		 log_debug2("added account: name=%s fetch=%s", a->name,
		     a->fetch->name);
	 }

%%

/* Programs */
