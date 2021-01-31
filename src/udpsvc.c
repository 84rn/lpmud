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
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "config.h"
#include "lint.h"
#include "main.h"
#include "interpret.h"
#include "simulate.h"
#include "ndesc.h"
#include "nqueue.h"
#include "net.h"

#ifdef CATCH_UDP_PORT

#ifndef INADDR_NONE
#define INADDR_NONE	0xffffffff
#endif

/*
 * UDP Service
 */

/*
 * Maximum UDP Datagram Size.
 */
#define	UDPSVC_RAWQ_SIZE	1024

static ndesc_t *udpsvc_nd = NULL;

/*
 * Send a UDP datagram.
 */
int
udpsvc_send(char *dest, int port, char *cp)
{
    struct sockaddr_in addr;
    int cc;

    if (udpsvc_nd == NULL || port < 0)
	return 0;

    memset(&addr, 0, sizeof (addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((u_short)port);
    addr.sin_addr.s_addr = inet_addr(dest);

    if (addr.sin_addr.s_addr == INADDR_NONE)
    {
#ifdef UDP_SEND_HOSTNAME
	struct hostent *hp;

	hp = gethostbyname(addr);
	if (hp == NULL)
	    return 0;
	memcpy(&addr.sin_addr, hp->h_addr, hp->h_length);
#else
	return 0;
#endif
    }

    cc = sendto(nd_fd(udpsvc_nd), cp, strlen(cp), 0,
	     (struct sockaddr *)&addr, sizeof (addr));

    return cc != -1;
}

/*
 * Read and process a UDP datagram.
 */
static void
udpsvc_read(ndesc_t *nd, nqueue_t *nq)
{
    int addrlen, cc;
    struct sockaddr_in addr;

    addrlen = sizeof (addr);

    cc = recvfrom(nd_fd(nd), nq_wptr(nq), nq_size(nq) - 1, 0,
	     (struct sockaddr *)&addr, &addrlen);

    if (cc == -1)
	return;

    nq_wptr(nq)[cc] = '\0';

    /* XXX We need a safe master apply XXX */
    push_string(inet_ntoa(addr.sin_addr), STRING_MSTRING);
    push_string((char *)nq_rptr(nq), STRING_MSTRING);
    (void)apply_master_ob(M_INCOMING_UDP, 2);
}

/*
 * Close the UDP Manager session and free the associated resources.
 */
static void
udpsvc_shutdown(ndesc_t *nd, nqueue_t *nq)
{
    (void)close(nd_fd(nd));
    nq_free(nq);
    nd_detach(nd);
    udpsvc_nd = NULL;
}

/*
 * Initialize the UDP Manager.
 */
void
udpsvc_init(int port)
{
    int s;
    struct sockaddr_in addr;
    nqueue_t *nq;

    s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (s == -1)
	fatal("udp_init: socket() error = %d.\n", errno);

    enable_reuseaddr(s);

    memset(&addr, 0, sizeof (addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((u_short)port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(s, (struct sockaddr *)&addr, sizeof (addr)) == -1)
    {
	if (errno == EADDRINUSE) 
	{
	    (void)fprintf(stderr, "UDP Socket already bound!\n");
	    debug_message("UDP Socket already bound!\n");
	    (void)close(s);
	    return;
	} 
	else 
	{
	    fatal("udp_init: bind() error = %d.\n", errno);
	}
    }

    enable_nbio(s);

    nq = nq_alloc(UDPSVC_RAWQ_SIZE);
    udpsvc_nd = nd_attach(s, udpsvc_read, NULL, NULL, NULL, udpsvc_shutdown,
		    nq);
    nd_enable(udpsvc_nd, ND_R);
}

#endif /* CATCH_UDP_PORT */
