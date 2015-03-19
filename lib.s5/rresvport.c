/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

/*
 * From the file: "@(#)rcmd.c   5.20 (Berkeley) 1/24/89";
 */

/*
 * Obtain a reserved TCP port.
 *
 * Note that we only create the socket and bind a local reserved port
 * to it.  It is the caller's responsibility to then connect the socket.
 */

#include	<sys/types.h>
#include	<sys/socket.h>
#include	<netinet/in.h>
#include	<sys/inet.h>
#include	<errno.h>
extern int	errno;

int			/* return socket descriptor, or -1 on error */
rresvport(alport)
int	*alport;	/* value-result; on entry = first port# to try */
			/* on return = actual port# we were able to bind */
{
	struct sockaddr_in	my_addr;
	int			sockfd;

	bzero((char *) &my_addr, sizeof(my_addr));
	my_addr.sin_family      = AF_INET;
	my_addr.sin_addr.s_addr = INADDR_ANY;

	if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		return(-1);

	for ( ; ; ) {
		my_addr.sin_port = htons((ushort) *alport);
		if (bind(sockfd, (struct sockaddr *) &my_addr, sizeof(my_addr)) >= 0)
			break;		/* OK, all done */

		if (errno != EADDRINUSE) {	/* some other error */
			close(sockfd);
			return(-1);
		}

		(*alport)--;	/* port already in use, try the next one */
		if (*alport == IPPORT_RESERVED / 2) {
			close(sockfd);
			errno = EAGAIN;
			return(-1);
		}
	}
	return(sockfd);
}
