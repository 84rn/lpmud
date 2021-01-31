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
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "config.h"
#include "lint.h"
#include "interpret.h"
#include "simulate.h"
#include "ndesc.h"
#include "nqueue.h"
#include "net.h"

#ifndef INADDR_LOOPBACK
#define	INADDR_LOOPBACK		0x7f000001
#endif

#ifndef EPROTO
#define	EPROTO	EPROTOTYPE
#endif

#ifdef SERVICE_PORT

/*
 * TCP Service
 */

/*
 * TCP Service Control Block.
 */
typedef struct {
    u_char	ts_flags;
    nqueue_t *	ts_canq;
    nqueue_t *	ts_rawq;
} tcpsvc_t;

/*
 * TCP Service Flags.
 */
#define	TF_CLOSE	0x01
#define	TF_ENABC	0x02
#define	TF_ENABR	0x04

/*
 * Queue Sizes.
 */
#define	TCPSVC_RAWQ_SIZE	1536
#define	TCPSVC_CANQ_SIZE	1536

/*
 * Maximum # of concurrent TCP Service.
 */
#define	TCPSVC_MAX		32

static ndesc_t *tcpsvc_nd = NULL;
static int tcpsvc_count = 0;

/*
 * Allocate a TCP Service control block.
 */
static tcpsvc_t *
tcpsvc_alloc(void)
{
    tcpsvc_t *tsp;

    tsp = xalloc(sizeof (tcpsvc_t));
    tsp->ts_flags = TF_ENABC;
    tsp->ts_rawq = nq_alloc(TCPSVC_RAWQ_SIZE);
    tsp->ts_canq = nq_alloc(TCPSVC_CANQ_SIZE);

    return tsp;
}

/*
 * Free a TCP Service control block.
 */
static void
tcpsvc_free(tcpsvc_t *tsp)
{
    nq_free(tsp->ts_rawq);
    nq_free(tsp->ts_canq);
    free(tsp);
}

/*
 * Process a disconnect indication.
 */
static void
tcpsvc_disconnect(ndesc_t *nd, tcpsvc_t *tsp)
{
    if (tsp->ts_flags & TF_CLOSE)
	return;

    tsp->ts_flags |= TF_CLOSE;
    nd_enable(nd, ND_C);
}

/*
 * Read the network into the raw input queue.
 */
static void
tcpsvc_read(ndesc_t *nd, tcpsvc_t *tsp)
{
    int cc;

    if (!nq_full(tsp->ts_rawq))
    {
	cc = nq_recv(tsp->ts_rawq, nd_fd(nd), NULL);
	if (cc == -1)
	{
	    switch (errno)
	    {
	    case EWOULDBLOCK:
	    case EINTR:
	    case EPROTO:
		break;

	    default:
		tcpsvc_disconnect(nd, tsp);
		return;
	    }
	}

	if (cc == 0)
	{
	    tcpsvc_disconnect(nd, tsp);
	    return;
	}

	if (tsp->ts_flags & TF_ENABC)
	{
	    tsp->ts_flags &= ~TF_ENABC;
	    nd_enable(nd, ND_C);
	}

	if (!nq_full(tsp->ts_rawq))
	    return;
    }

    tsp->ts_flags |= TF_ENABR;
    nd_disable(nd, ND_R);
}

/*
 * Write the contents of the canonical queue to the network.
 */
static void
tcpsvc_write(ndesc_t *nd, tcpsvc_t *tsp)
{
    if (!nq_empty(tsp->ts_canq))
    {
	if (nq_send(tsp->ts_canq, nd_fd(nd), NULL) == -1)
	{
	    switch (errno)
	    {
	    case EWOULDBLOCK:
	    case EINTR:
	    case EPROTO:
		break;

	    default:
		tcpsvc_disconnect(nd, tsp);
		return;
	    }
	}

	if (!nq_empty(tsp->ts_canq))
	    return;
    }

    nq_init(tsp->ts_canq);

    nd_disable(nd, ND_W);
    nd_enable(nd, ND_C);
}

/*
 * Close a TCP Service connection and free the associated resources.
 */
static void
tcpsvc_shutdown(ndesc_t *nd, tcpsvc_t *tsp)
{
    if (tsp != NULL)
    {
	tcpsvc_free(tsp);
	tcpsvc_count--;
    }
    else
    {
	tcpsvc_nd = NULL;
    }

    (void)close(nd_fd(nd));
    nd_detach(nd);
}

/*
 * Perform TCP Service clean-up processing.
 */
static void
tcpsvc_cleanup(ndesc_t *nd, tcpsvc_t *tsp)
{
    u_char c;
    struct svalue *svp;
    struct gdexception exception_frame;

    if (tsp->ts_flags & TF_CLOSE)
    {
	tcpsvc_shutdown(nd, tsp);
    }
    else
    {
	for (;;)
	{
	    if (nq_empty(tsp->ts_rawq))
	    {
		nq_init(tsp->ts_rawq);
		if (tsp->ts_flags & TF_ENABR)
		{
		    tsp->ts_flags &= ~TF_ENABR;
		    nd_enable(nd, ND_R);
		}
		tsp->ts_flags |= TF_ENABC;
		nd_disable(nd, ND_C);
		return;
	    }
	    c = nq_getc(tsp->ts_rawq);
	    if (c == '\n')
		break;
	    if (!nq_full(tsp->ts_canq))
		nq_putc(tsp->ts_canq, c);
	}

	nd_disable(nd, ND_C);
	nd_enable(nd, ND_W);
	
	if (nq_full(tsp->ts_canq))
	{
	    nq_init(tsp->ts_canq);
	    nq_puts(tsp->ts_canq, (u_char *)"ERROR Service request too long.\n");
	    tcpsvc_disconnect(nd, tsp);
	    return;
	}

	nq_putc(tsp->ts_canq, '\0');

	exception_frame.e_exception = exception;
	exception_frame.e_catch = 0;

	exception = &exception_frame;

	if (setjmp(exception_frame.e_context) == 0)
	{
	    push_string((char *)nq_rptr(tsp->ts_canq), STRING_MSTRING);
	    svp = apply_master_ob(M_INCOMING_SERVICE, 1);
	}
	else
	{
	    svp = NULL;
	}

	exception = exception->e_exception;

	nq_init(tsp->ts_canq);

	if (svp == NULL || svp->type != T_STRING)
	{
	    nq_puts(tsp->ts_canq, (u_char *)"ERROR Service calls not supported.\n");
	    tcpsvc_disconnect(nd, tsp);
	    return;
	}

	if (strlen(svp->u.string) > nq_size(tsp->ts_canq))
	{
	    nq_puts(tsp->ts_canq, (u_char *)"ERROR Service response too long.\n");
	    tcpsvc_disconnect(nd, tsp);
	    return;
	}

	nq_puts(tsp->ts_canq, (u_char *)svp->u.string);
    }
}

/*
 * Accept a TCP Service connection.
 */
static void
tcpsvc_accept(ndesc_t *nd, void *vp)
{
    int s, addrlen;
    struct sockaddr_in addr;
    tcpsvc_t *tsp;

    addrlen = sizeof (addr);

    s = accept(nd_fd(nd), (struct sockaddr *)&addr, &addrlen);
    if (s == -1)
    {
	switch (errno)
	{
	default:
	    fatal("svc_server: accept() errno = %d.\n", errno);
	case EWOULDBLOCK:
	case EINTR:
	case EPROTO:
	case ECONNRESET:
	    return;
	}
    }

    if (addr.sin_addr.s_addr != htonl(INADDR_LOOPBACK))
    {
#ifdef ALLOWED_SERVICE
	if (addr.sin_addr.s_addr != htonl(ALLOWED_SERVICE))
#endif
	{
	    (void)fprintf(stderr, "SERVICE PORT ACCESS FROM %s %d\n",
		inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
	    (void)close(s);
	    return;
	}
    }

    enable_nbio(s);

    tsp = tcpsvc_alloc();
    nd = nd_attach(s, tcpsvc_read, tcpsvc_write, NULL, tcpsvc_cleanup,
	     tcpsvc_shutdown, tsp);

    if (++tcpsvc_count > TCPSVC_MAX)
    {
	nq_puts(tsp->ts_canq, (u_char *)"ERROR Too many services in use.\n");
	nd_enable(nd, ND_W);
	tcpsvc_disconnect(nd, tsp);
	return;
    }

    nd_enable(nd, ND_R);
}

/*
 * Initialize the TCP Service Manager.
 */
void
tcpsvc_init(void)
{
    extern int service_port;
    int s;
    struct sockaddr_in addr;

    if (service_port < 0)
	return;

    s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == -1)
	return;

    enable_reuseaddr(s);

    memset(&addr, 0, sizeof (addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((u_short)service_port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(s, (struct sockaddr *)&addr, sizeof (addr)) == -1)
    {
	(void)close(s);
	return;
    }

    if (listen(s, 5) == -1)
    {
	(void)close(s);
	return;
    }

    enable_nbio(s);

    tcpsvc_nd = nd_attach(s, tcpsvc_accept, NULL, NULL, NULL,
		    tcpsvc_shutdown, NULL);
    nd_enable(tcpsvc_nd, ND_R);
}

#endif
