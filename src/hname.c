/*-
 * Copyright (c) 1997 Dave Richards <dave@synergy.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * and exceptions are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the authors may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 * 4. The code can not be used by Gary Random, Random Communications, Inc.,
 *    the employees of Random Communications, Inc. or its subsidiaries,
 *    including Defiance MUD, without prior written permission from the
 *    authors.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL
 * THE AUTHORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "config.h"
#include "lint.h"
#include "net.h"
#include "comm.h"
#include "nqueue.h"
#include "ndesc.h"
#include "hname.h"

#ifndef INADDR_LOOPBACK
#define	INADDR_LOOPBACK		0x7f000001
#endif

#ifndef INADDR_NONE
#define	INADDR_NONE		0xffffffff
#endif

#ifndef EPROTO
#define EPROTO EPROTOTYPE
#endif

extern struct interactive *all_players[MAX_PLAYERS];

#ifndef NO_IP_DEMON

/*
 * Hostname Server
 */

/*
 * Hostname Server Control Block.
 */
typedef struct {
    u_char	h_flags;
    nqueue_t *	h_rawq;
    nqueue_t *	h_canq;
    nqueue_t *	h_outq;
} hname_t;

/*
 * Hostname Server Flags.
 */
#define	HF_CLOSE	0x01
#define	HF_ENABR	0x02
#define	HF_ENABW	0x04

/*
 * Queue Sizes.
 */
#define	HNAME_RAWQ_SIZE	128
#define	HNAME_CANQ_SIZE	128
#define	HNAME_OUTQ_SIZE	128

static ndesc_t *hname_nd = NULL;

/*
 * Allocate a Hostname Server control block.
 */
static hname_t *
hname_alloc(void)
{
    hname_t *hp;

    hp = xalloc(sizeof (hname_t));
    hp->h_flags = HF_ENABW;
    hp->h_rawq = nq_alloc(HNAME_RAWQ_SIZE);
    hp->h_canq = nq_alloc(HNAME_CANQ_SIZE);
    hp->h_outq = nq_alloc(HNAME_OUTQ_SIZE);

    return hp;
}

/*
 * Free a Hostname Server control block.
 */
static void
hname_free(hname_t *hp)
{
    nq_free(hp->h_rawq);
    nq_free(hp->h_canq);
    nq_free(hp->h_outq);
    free(hp);
}

/*
 * Process a response from the hname server.
 */
static void
hname_input(char *cp)
{
    int ntok, lport, rport, i;
    char *tok[5];
    u_long addr;
    struct interactive *ip;
    static char *sep[5] = { " ", ",\n", ",", ":", "\n" };

    for (ntok = 0; ntok < 5; ntok++)
    {
	cp = strtok(ntok == 0 ? cp : NULL, sep[ntok]);
	if (cp == NULL)
	    break;
	tok[ntok] = cp;
    }

    if (ntok != 2 && ntok != 4 && ntok != 5)
	return;

    addr = inet_addr(tok[0]);
    if (addr == INADDR_NONE)
	return;

    add_ip_entry(addr, tok[1]);

    if (ntok != 5)
	return;

    lport = atoi(tok[2]);
    rport = atoi(tok[3]);

    for (i = 0; i < MAX_PLAYERS; i++)
    {
	ip = all_players[i];
	if (ip == NULL)
	    continue;

	if (ip->addr.sin_addr.s_addr == addr &&
	    ip->lport == lport &&
	    ip->rport == rport)
	{
	    if (ip->rname != NULL)
		free(ip->rname);
	    ip->rname = xalloc(strlen(tok[4]) + 1);
	    strcpy(ip->rname, tok[4]);
	    return;
	}
    }
}

/*
 * Send a request to the hname server.
 */
void
hname_sendreq(char *addr, u_short lport, u_short rport)
{
    char req[32];
    hname_t *hp;

    if (hname_nd == NULL)
	return;

    hp = nd_vp(hname_nd);

    sprintf(req, "%s;%u,%u\n", addr, lport, rport);

    if (nq_avail(hp->h_outq) < strlen(req))
	return;

    nq_puts(hp->h_outq, (u_char *)req);

    if (hp->h_flags & HF_ENABW)
    {
	hp->h_flags &= ~HF_ENABW;
	nd_enable(hname_nd, ND_W);
    }
}

/*
 * Process a network disconnect indication.
 */
static void
hname_disconnect(ndesc_t *nd, hname_t *hp)
{
    if (hp->h_flags & HF_CLOSE)
	return;

    hp->h_flags |= HF_CLOSE;
    nd_enable(nd, ND_C);
}

/*
 * Read the network into the raw input queue.
 */
static void
hname_read(ndesc_t *nd, hname_t *hp)
{
    int cc;

    if (!nq_full(hp->h_rawq))
    {
	cc = nq_recv(hp->h_rawq, nd_fd(nd), NULL);
	if (cc == -1)
	{
	    switch (errno)
	    {
	    case EWOULDBLOCK:
	    case EINTR:
	    case EPROTO:
		break;

	    default:
		hname_disconnect(nd, hp);
		return;
	    }
	}

	if (cc == 0)
	{
	    hname_disconnect(nd, hp);
	    return;
	}

	nd_enable(nd, ND_C);

	if (!nq_full(hp->h_rawq))
	    return;
    }

    hp->h_flags |= HF_ENABR;
    nd_disable(nd, ND_R);
}

/*
 * Write the contents of the output queue to the network.
 */
static void
hname_write(ndesc_t *nd, hname_t *hp)
{
    if (!nq_empty(hp->h_outq))
    {
	if (nq_send(hp->h_outq, nd_fd(nd), NULL) == -1)
	{
	    switch (errno)
	    {
	    case EWOULDBLOCK:
	    case EINTR:
	    case EPROTO:
		break;

	    default:
		hname_disconnect(nd, hp);
		return;
	    }
	}

	if (!nq_empty(hp->h_outq))
	    return;
    }

    nq_init(hp->h_outq);

    hp->h_flags |= HF_ENABW;
    nd_disable(nd, ND_W);
}

/*
 * Close the Hostname Service connection and free the associated resources.
 */
static void
hname_shutdown(ndesc_t *nd, hname_t *hp)
{
    (void)close(nd_fd(nd));
    nd_detach(nd);
    hname_free(hp);
    hname_nd = NULL;
}

/*
 * Perform Hostname Server clean-up processing.
 */
static void
hname_cleanup(ndesc_t *nd, hname_t *hp)
{
    u_char c;

    if (hp->h_flags & HF_CLOSE)
    {
	hname_shutdown(nd, hp);
    }
    else
    {
	for (;;)
	{
	    if (nq_empty(hp->h_rawq))
	    {
		nq_init(hp->h_rawq);
		if (hp->h_flags & HF_ENABR)
		{
		    hp->h_flags &= ~HF_ENABR;
		    nd_enable(nd, ND_R);
		}
		nd_disable(nd, ND_C);
		return;
	    }
	    c = nq_getc(hp->h_rawq);
	    if (c == '\n')
		break;
	    if (!nq_full(hp->h_canq))
		nq_putc(hp->h_canq, c);
	}

	if (!nq_full(hp->h_canq))
	{
	    nq_putc(hp->h_canq, '\0');
	    hname_input((char *)nq_rptr(hp->h_canq));
	}

	nq_init(hp->h_canq);
    }
}

/*
 * Initialize the Hostname Server.
 */
void
hname_init()
{
    int s, ns, addrlen, pid;
    struct sockaddr_in addr;
    char path[MAXPATHLEN];
    hname_t *hp;

    s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == -1)
	return;

    memset(&addr, 0, sizeof (addr));
    addr.sin_family = AF_INET;
    addr.sin_port = 0;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    if (bind(s, (struct sockaddr *)&addr, sizeof (addr)) == -1)
    {
	(void)close(s);
	return;
    }

    addrlen = sizeof (addr);

    if (getsockname(s, (struct sockaddr *)&addr, &addrlen) == -1)
    {
	(void)close(s);
	return;
    }

    if (listen(s, 1) == -1)
    {
	(void)close(s);
	return;
    }

    pid = fork();
    if (pid == -1)
    {
	(void)close(s);
	return;
    }

    if (pid == 0)
    {
	ns = accept(s, (struct sockaddr *)&addr, &addrlen);
	if (ns == -1)
	{
	    (void)close(s);
	    exit(1);
	}

	(void)close(s);

	(void)dup2(ns, 0);
	(void)dup2(ns, 1);

	for (s = 3; s < FD_SETSIZE; s++)
	    (void)close(s);

	strncpy(path, BINDIR, sizeof (path));
	strncat(path, "/hname", sizeof (path));

	(void)execl(path, "hname", 0);

        (void)fprintf(stderr, "exec of hname failed.\n");

	exit(1);
    }
    else
    {
	(void)close(s);

	s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (s == -1)
	{
	    kill(pid, SIGKILL);
	    return;
	}

	if (connect(s, (struct sockaddr *)&addr, sizeof (addr)) == -1)
	{
	    (void)close(s);
	    kill(pid, SIGKILL);
	    return;
	}

	enable_nbio(s);

	hp = hname_alloc();
	hname_nd = nd_attach(s, hname_read, hname_write, NULL,
		       hname_cleanup, hname_shutdown, hp);
	nd_enable(hname_nd, ND_R);
    }
}

#endif
