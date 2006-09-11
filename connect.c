/* $Id: connect.c,v 1.22 2006-09-11 07:32:02 nicm Exp $ */

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
#include <sys/socket.h> 

#include <errno.h>
#include <limits.h>
#include <netdb.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <openssl/ssl.h>
#include <openssl/err.h>

#include "fdm.h"

int	httpproxy(struct server *, struct proxy *, struct io *, char **);
int	socks5proxy(struct server *, struct proxy *, struct io *, char **);
int	getport(char *);

struct proxy *
getproxy(char *url)
{
	struct proxy		*pr;
	char			*ptr, *end;
	struct {
		char		*proto;
		enum proxytype	 type;
		int		 ssl;
		char		*port;
	} *proxyent, proxylist[] = {
		{ "http://",    PROXY_HTTP,   0, "http" },
		{ "https://",   PROXY_HTTPS,  1, "https" },
		{ "socks://",   PROXY_SOCKS5, 0, "socks" },
		{ "socks5://",  PROXY_SOCKS5, 0, "socks" },
		{ NULL,	        0,	      0, NULL }
	};

	/* find proxy */
	for (proxyent = proxylist; proxyent->proto != NULL; proxyent++) {
		if (strncmp(url, proxyent->proto, strlen(proxyent->proto)) == 0)
			break;
	}
	if (proxyent->proto == NULL)
		return (NULL);

	pr = xmalloc(sizeof *pr);
	pr->type = proxyent->type;
	pr->server.ssl = proxyent->ssl;
	pr->server.port = xstrdup(proxyent->port);
	pr->server.ai = NULL;
	url += strlen(proxyent->proto);

	/* strip trailing /s */
	ptr = url + strlen(url) - 1;
	while (ptr > url && *ptr == '/')
		*ptr-- = '\0';
	if (*url == '\0') {
		xfree(pr->server.port);
		xfree(pr);
		return (NULL); 
	}

	pr->user = pr->pass = NULL;
	if ((end = strchr(url, '@')) != NULL) {
		ptr = strchr(url, ':');
		if (ptr != NULL && ptr < end) {
			*ptr++ = '\0';
			pr->user = strdup(url);
		}
		*end++ = '\0';
		pr->pass = strdup(ptr);
		url = end;
	}
	
	if ((ptr = strchr(url, ':')) != NULL) {
		xfree(pr->server.port);
		*ptr++ = '\0';
		if (*ptr == '\0') {
			if (pr->user != NULL)
				xfree(pr->user);
			if (pr->pass != NULL)
				xfree(pr->pass);
			xfree(pr);
			return (NULL); 
		}
		pr->server.port = xstrdup(ptr);
	}

	pr->server.host = xstrdup(url);

	return (pr);
}

struct io *
connectproxy(struct server *srv, struct proxy *pr, const char eol[2], 
    char **cause)
{
	struct io	*io;

	if (pr == NULL)
		return (connectio(srv, eol, cause));

	io = connectio(&pr->server, IO_CRLF, cause);
	if (io == NULL)
		return (NULL);

	switch (pr->type) {
	case PROXY_HTTP:
		if (httpproxy(srv, pr, io, cause) != 0) {
			io_close(io);
			io_free(io);
			return (NULL);
		}
		break;
	case PROXY_SOCKS5:
		if (socks5proxy(srv, pr, io, cause) != 0) {
			io_close(io);
			io_free(io);
			return (NULL);
		}
		break;
	default:
		fatalx("unknown proxy type");
	}

	io->eol = eol;
	return (io);
}

int
getport(char *port)
{
	struct servent	*sv;
	int	         n;
	const char	*errstr;

	sv = getservbyname(port, NULL);
	if (sv == NULL) {
		n = strtonum(port, 1, UINT16_MAX, &errstr);
		if (errstr != NULL) {
			endservent();
			return (-1);
		}
	} else
		n = ntohs(sv->s_port);
	endservent();

	return (n);
}

int
socks5proxy(struct server *srv, struct proxy *pr, struct io *io, char **cause)
{
	int	port, auth;
	char	buf[32], *ptr;
	size_t	len;

	if ((port = getport(srv->port)) < 0) {
		xasprintf(cause, "bad port: %s", srv->port);
		return (1);
	}	

	/* method selection */
	auth = pr->user != NULL && pr->pass != NULL;
	buf[0] = 5;
	buf[1] = auth ? 2 : 1;
	buf[2] = 0;	/* 0 = no auth */
	buf[3] = 2;	/* 2 = user/pass auth */
	io_write(io, buf, auth ? 4 : 3);
	if (io_wait(io, 2, cause) != 0)
		return (1);
	io_read2(io, buf, 2);
	if (buf[0] != 5) {
		xasprintf(cause, "bad protocol version: %d", buf[0]);
		return (1);
	}
	if ((buf[1] != 0 && buf[1] != 2) || (auth == 0 && buf[1] == 2)) {
		xasprintf(cause, "unexpected method: %d", buf[1]);
		return (1);
	}

	/* user/pass negotiation */
	if (buf[1] == 2) {
		ptr = buf;
		*ptr++ = 5;
		len = strlen(pr->user);
		*ptr++ = len;
		memcpy(ptr, pr->user, len);
		ptr += len;
		len = strlen(pr->pass);
		*ptr++ = len;
		memcpy(ptr, pr->pass, len);
		ptr += len;
		io_write(io, buf, ptr - buf);

		io_read2(io, buf, 2);
		if (io_wait(io, 2, cause) != 0)
			return (1);
		if (buf[0] != 5) {
			xasprintf(cause, "bad protocol version: %d", buf[0]);
			return (1);
		}
		if (buf[1] != 0) {
			xasprintf(cause, "authentication failed");
			return (1);
		}
	}

	/* connect request */
	ptr = buf;
	*ptr++ = 5;
	*ptr++ = 1; /* 1 = connect */
	*ptr++ = 0; /* reserved */
	*ptr++ = 3; /* 3 = domain name */
	len = strlen(srv->host);
	*ptr++ = len;
	memcpy(ptr, srv->host, len);
	ptr += len;
	*((u_int16_t *) ptr) = htons(port);
	ptr += 2;
	io_write(io, buf, ptr - buf);

	/* connect response */
	if (io_wait(io, 5, cause) != 0)
		return (1);
	io_read2(io, buf, 5);
	if (buf[0] != 5) {
		xasprintf(cause, "bad protocol version: %d", buf[0]);
		return (1);
	}
	switch (buf[1]) {
	case 0:
		break;
	case 1:
		xasprintf(cause, "%d: server failure", buf[1]);
		return (1);
	case 2:
		xasprintf(cause, "%d: connection not permitted", buf[1]);
		return (1);
	case 3:
		xasprintf(cause, "%d: network unreachable", buf[1]);
		return (1);
	case 4:
		xasprintf(cause, "%d: host unreachable", buf[1]);
		return (1);
	case 5:
		xasprintf(cause, "%d: connection refused", buf[1]);
		return (1);
	case 6:
		xasprintf(cause, "%d: TTL expired", buf[1]);
		return (1);
	case 7:
		xasprintf(cause, "%d: command not supported", buf[1]);
		return (1);
	case 8:
		xasprintf(cause, "%d: address type not supported", buf[1]);
		return (1);
	default:
		xasprintf(cause, "%d: unknown failure", buf[1]);
		return (1);
	}
	
	/* flush the rest */
	switch (buf[3]) {
	case 1: /* IPv4 */
		len = 5;
		break;
	case 3: /* IPv6 */
		len = 17;
		break;
	case 4: /* host */
		len = buf[4] + 2;
		break;
	default:
		xasprintf(cause, "unknown address type: %d", buf[3]);
		return (1);
	}
	if (io_wait(io, len, cause) != 0)
		return (1);	
	io_read2(io, buf, len);

	return (0);
}

int
httpproxy(struct server *srv, struct proxy *pr, struct io *io, char **cause)
{
	char		*line;
	int		 port, header;

	if (pr->user != NULL || pr->pass != NULL) {
		xasprintf(cause, "HTTP proxy authentication is not supported");
		return (1);
	}

	if ((port = getport(srv->port)) < 0) {
		xasprintf(cause, "bad port: %s", srv->port);
		return (1);
	}

	io_writeline(io, "CONNECT %s:%d HTTP/1.1", srv->host, port);
	io_writeline(io, NULL);

	header = 0;
	for (;;) {
		if (io_poll(io, cause) != 1)
			return (1);
		
		for (;;) {
			line = io_readline(io);
			if (line == NULL)
				break;

			if (header == 0) {
				if (strlen(line) < 12 || 
				    strncmp(line, "HTTP/", 5) != 0 ||
				    strncmp(line + 8, " 200", 4) != 0) {
					xfree(line);
					xasprintf(cause, "unexpected data: %s",
					    line);
					return (1);
				}
				header = 1;
			} else {
				if (*line == '\0')
					return (0);
			}

			xfree(line);
		}
	}
}

struct io *
connectio(struct server *srv, const char eol[2], char **cause)
{
	int		 fd = -1, error = 0, n;
	struct addrinfo	 hints;
	struct addrinfo	*ai;
	char		*fn = NULL;
	SSL_CTX		*ctx;
	SSL		*ssl;

	if (srv->ai == NULL) {
		memset(&hints, 0, sizeof hints);
		hints.ai_family = PF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;
		error = getaddrinfo(srv->host, srv->port, &hints, &srv->ai);
		if (error != 0) {
			*cause = xstrdup(gai_strerror(error));
			return (NULL);
		}
	}

	for (ai = srv->ai; ai != NULL; ai = ai->ai_next) {
		fd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
		if (fd < 0) {
			fn = "socket";
			continue;
		}
		if (connect(fd, ai->ai_addr, ai->ai_addrlen) < 0) {
			error = errno;
			close(fd);
			errno = error;
			fd = -1;
			fn = "connect";
			continue;
		}
		break;
	}

	if (fd < 0) {
		xasprintf(cause, "%s: %s", fn, strerror(errno));
		return (NULL);
	}
	if (!srv->ssl)
		return (io_create(fd, NULL, eol));

	ctx = SSL_CTX_new(SSLv23_client_method());
	SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, NULL);	
	
	ssl = SSL_new(ctx);
	if (ssl == NULL) {
		close(fd);
		xasprintf(cause, "SSL_new: %s", SSL_err());
		return (NULL);
	}

	if (SSL_set_fd(ssl, fd) != 1) {
		close(fd);
		xasprintf(cause, "SSL_set_fd: %s", SSL_err());
		return (NULL);
	}

	SSL_set_connect_state(ssl);
	if ((n = SSL_connect(ssl)) < 1) {
		close(fd);
		n = SSL_get_error(ssl, n);
		xasprintf(cause, "SSL_connect: %d: %s", n, SSL_err());
		return (NULL);
	}		

	return (io_create(fd, ssl, eol));
}
