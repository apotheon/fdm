/* $Id: array.h,v 1.3 2007-05-08 19:24:49 nicm Exp $ */

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

#ifndef ARRAY_H
#define ARRAY_H

#define ARRAY_DECL(n, c)						\
	struct n {							\
		c	*list;						\
		u_int	 num;						\
		size_t	 space;						\
	}

#define ARRAY_INIT(a) do {						\
	(a)->num = 0;							\
	(a)->list = NULL;		 				\
	(a)->space = 0;							\
} while (0)

#define ARRAY_ADD(a, s, c) do {						\
	ENSURE_SIZE2((a)->list, (a)->space, (a)->num + 1, sizeof (c));	\
	(a)->list[(a)->num] = s;					\
	(a)->num++;							\
} while (0)

#define ARRAY_SET(a, i, s) do {					\
	if (((u_int) (i)) >= (a)->num) {				\
		log_warnx("ARRAY_SET: bad index: %u, at %s:%d",		\
		    i, __FILE__, __LINE__);				\
		exit(1);						\
	}								\
	(a)->list[i] = s;						\
} while (0)

#define ARRAY_REMOVE(a, i, c) do {					\
	if (((u_int) (i)) >= (a)->num) {				\
		log_warnx("ARRAY_REMOVE: bad index: %u, at %s:%d",	\
		    i, __FILE__, __LINE__);				\
		exit(1);						\
	}								\
	if (i < (a)->num - 1) {						\
		c 	*aptr = (a)->list + i;				\
		memmove(aptr, aptr + 1, (sizeof (c)) * ((a)->num - (i) - 1)); \
	}								\
	(a)->num--;							\
        if ((a)->num == 0)						\
		ARRAY_FREE(a);						\
} while (0)

#define ARRAY_EXPAND(a, n, c) do {					\
	ENSURE_SIZE2((a)->list, (a)->space, (a)->num + n, sizeof (c));	\
	(a)->num += n;							\
} while (0)

#define ARRAY_TRUNC(a, n, c) do {					\
	if ((a)->num > n)						\
		(a)->num -= n;				       		\
	else								\
		ARRAY_FREE(a);						\
} while (0)

#define ARRAY_CONCAT(a, b, c) do {					\
	ENSURE_SIZE2((a)->list, (a)->space, (a)->num + (b)->num, sizeof (c)); \
	memcpy((a)->list + (a)->num, (b)->list, (b)->num * (sizeof (c)));     \
	(a)->num += (b)->num;						\
} while (0)

#define ARRAY_EMPTY(a) ((a) == NULL || (a)->num == 0)
#define ARRAY_LENGTH(a) ((a)->num)
#define ARRAY_LAST(a) ARRAY_ITEM(a, (a)->num - 1)
#define ARRAY_ITEM(a, n) ((a)->list[n])

#define ARRAY_FREE(a) do {						\
	if ((a)->list != NULL)						\
		xfree((a)->list);					\
	ARRAY_INIT(a);							\
} while (0)
#define ARRAY_FREEALL(a) do {						\
	ARRAY_FREE(a);							\
	xfree(a);							\
} while (0)

#endif
