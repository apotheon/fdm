/* $Id: cache-op.c,v 1.1 2007-10-09 11:38:36 nicm Exp $ */

/*
 * Copyright (c) 2007 Nicholas Marriott <nicm@users.sourceforge.net>
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

#include <string.h>

#include "fdm.h"

__dead void	cache_op_add(int, char **);
__dead void	cache_op_remove(int , char **);
__dead void	cache_op_list(int, char **);
__dead void	cache_op_dump(int, char **);
__dead void	cache_op_clear(int, char **);

__dead void
cache_op(int argc, char **argv)
{
	char	*cmd;

	if (argc < 1)
		usage();
	cmd = argv[0];
	argc--;
	argv++;

	if (strncmp(cmd, "add", strlen(cmd)) == 0)
		cache_op_add(argc, argv);
	if (strncmp(cmd, "remove", strlen(cmd)) == 0)
		cache_op_remove(argc, argv);
	if (strncmp(cmd, "list", strlen(cmd)) == 0)
		cache_op_list(argc, argv);
	if (strncmp(cmd, "dump", strlen(cmd)) == 0)
		cache_op_dump(argc, argv);
	if (strncmp(cmd, "clear", strlen(cmd)) == 0)
		cache_op_clear(argc, argv);
	usage();
}

__dead void
cache_op_add(int argc, char **argv)
{
	TDB_CONTEXT	*db;

	if (argc != 2)
		usage();

	if ((db = db_open(argv[0])) == NULL) {
		log_warn("%s", argv[0]);
		exit(1);
	}
	
	if (db_add(db, argv[1]) != NULL) {
		log_warnx("%s: cache error", argv[0]);
		exit(1);
	}

	exit(0);
}

__dead void
cache_op_remove(int argc, char **argv)
{
	TDB_CONTEXT	*db;

	if (argc != 2)
		usage();

	if ((db = db_open(argv[0])) == NULL) {
		log_warn("%s", argv[0]);
		exit(1);
	}
	
	if (!db_contains(db, argv[1])) {
		log_warnx("%s: key not found: %s", argv[0], argv[1]);
		exit(1);
	}
	if (db_remove(db, argv[1]) != NULL) {
		log_warnx("%s: cache error", argv[0]);
		exit(1);
	}

	exit(0);
}

__dead void
cache_op_list(int argc, char **argv)
{
	struct cache	*cache;
	TDB_CONTEXT	*db;

	if (argc == 1) {
		if ((db = db_open(argv[0])) == NULL) {
			log_warn("%s", argv[0]);
			exit(1);
		}

		log_info("%s: %u keys", argv[0], db_size(db));	
		
		db_close(db);
		exit(0);
	}

	if (argc != 0)
		usage();

	TAILQ_FOREACH(cache, &conf.caches, entry) {
		if ((cache->db = db_open(cache->path)) == NULL) {
			log_warn("%s", cache->path);
			exit(1);
		}

		log_info("%s: %u keys", cache->path, db_size(cache->db));

		db_close(cache->db);
	}

	exit(0);
}

__dead void
cache_op_dump(int argc, char **argv)
{
	TDB_CONTEXT	*db;

	if (argc != 1)
		usage();

	if ((db = db_open(argv[0])) == NULL) {
		log_warn("%s", argv[0]);
		exit(1);
	}
	
	if (db_print(db, log_info) != 0) {
		log_warnx("%s: cache error", argv[0]);
		exit(1);
	}

	exit(0);
}

__dead void
cache_op_clear(int argc, char **argv)
{
	TDB_CONTEXT	*db;

	if (argc != 1)
		usage();

	if ((db = db_open(argv[0])) == NULL) {
		log_warn("%s", argv[0]);
		exit(1);
	}
	
	if (db_clear(db) != 0) {
		log_warnx("%s: cache error", argv[0]);
		exit(1);
	}

	exit(0);
}

