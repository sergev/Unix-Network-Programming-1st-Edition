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

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)rcmd.c	5.20 (Berkeley) 1/24/89";
#endif /* LIBC_SCCS and not lint */

#include	<sys/types.h>
#include	<sys/socket.h>
#include	<sys/file.h>
#include	<sys/signal.h>
#include	<netinet/in.h>
#include	<arpa/inet.h>

#include	<stdio.h>
#include	<netdb.h>
#include	<errno.h>
extern int	errno;

int					/* return socket descriptor - sockfd1 */
rcmd(ahost, rport, cliuname, servuname, cmd, fd2ptr)
char	**ahost;	/* pointer to address of host name */
u_short	rport;		/* port on server to connect to - network byte order */
char	*cliuname;	/* username on client system (i.e., caller's username */
char	*servuname;	/* username to use on server system */
char	*cmd;		/* command string to execute on server */
int	*fd2ptr;	/* ptr to secondary socket descriptor (if not NULL) */
{
	int			sockfd1, timo, lport;
	long			oldmask;
	char			c;
	struct sockaddr_in	serv_addr, serv2_addr;
	struct hostent		*hp;
	fd_set			readfds;

	if ( (hp = gethostbyname(*ahost)) == NULL) {
		herror(*ahost);
		return(-1);
	}
	*ahost = hp->h_name;	/* return hostname we're using to caller */

	oldmask = sigblock(sigmask(SIGURG));

	lport = IPPORT_RESERVED - 1;
	timo  = 1;
	for ( ; ; ) {
		if ( (sockfd1 = rresvport(&lport)) < 0) {
			if (errno == EAGAIN)
				fprintf(stderr, "socket: All ports in use\n");
			else
				perror("rcmd: socket");
			sigsetmask(oldmask);
			return(-1);
		}

		fcntl(sockfd1, F_SETOWN, getpid());
					/* set pid for socket signals */

		/*
		 * Fill in the socket address of the server, and connect to
		 * the server.
		 */

		bzero((char *) &serv_addr, sizeof(serv_addr));
		serv_addr.sin_family = hp->h_addrtype;
		bcopy(hp->h_addr_list[0], (caddr_t)&serv_addr.sin_addr,
							hp->h_length);
		serv_addr.sin_port = rport;
		if (connect(sockfd1, (struct sockaddr *) &serv_addr,
						sizeof(serv_addr)) >= 0)
			break;		/* OK, continue onward */

		close(sockfd1);
		if (errno == EADDRINUSE) {
			/*
			 * We were able to bind the local address, but couldn't
			 * connect to the server.  Decrement the starting
			 * port number for rresvport() and try again.
			 */

			lport--;
			continue;
		}

		if (errno == ECONNREFUSED && timo <= 16) {
			/*
			 * The connection was refused.  The server's system
			 * is probably overloaded.  Sleep for a while, then
			 * try again.  We try this 5 times (total of 31 sec).
			 */

			sleep(timo);
			timo *= 2;	/* increase timer: 1, 2, 4, 8, 16 sec */
			continue;
		}

		if (hp->h_addr_list[1] != NULL) {
			/*
			 * If there's another address for the host, try it.
			 */

			int	oerrno;

			oerrno = errno;	/* save errno over call to fprintf */
			fprintf(stderr, "connect to address %s: ",
						inet_ntoa(serv_addr.sin_addr));
			errno = oerrno;
			perror((char *) 0);

			hp->h_addr_list++;    /* incr. pointer for next time */
			bcopy(hp->h_addr_list[0], (caddr_t) &serv_addr.sin_addr,
						hp->h_length);
			fprintf(stderr, "Trying %s...\n",
						inet_ntoa(serv_addr.sin_addr));
			continue;
		}

		perror(hp->h_name);	/* none of the above, quit */
		sigsetmask(oldmask);
		return(-1);
	}

	if (fd2ptr == (int *) 0) {
		/*
		 * Caller doesn't want a secondary channel.  Write a byte
		 * of 0 to the socket, to let the server know this.
		 */

		write(sockfd1, "", 1);
		lport = 0;

	} else {
		/*
		 * Create the secondary socket and connect it to the
		 * server also.  We have to bind the secondary socket to
		 * a reserved TCP port also.
		 */

		char	num[8];
		int	socktemp, sockfd2, len;

		lport--;	/* decrement for starting port# */
		if ( (socktemp = rresvport(&lport)) < 0)
			goto bad;

		listen(socktemp, 1);

		/*
		 * Write an ASCII string with the port number to the server,
		 * so it knows which port to connect to.
		 */

		sprintf(num, "%d", lport);
		if (write(sockfd1, num, strlen(num)+1) != strlen(num)+1) {
			perror("write: setting up stderr");
			close(socktemp);
			goto bad;
		}

		FD_ZERO(&readfds);
		FD_SET(sockfd1, &readfds);
		FD_SET(socktemp, &readfds);
		errno = 0;
		if ((select(32, &readfds, (fd_set *) 0, (fd_set *) 0,
						(struct timeval *) 0) < 1) ||
		    !FD_ISSET(socktemp, &readfds)) {
			if (errno != 0)
				perror("select: setting up stderr");
			else
			    fprintf(stderr,
				"select: protocol failure in circuit setup.\n");
			close(socktemp);
			goto bad;
		}

		/*
		 * The server does the connect() to us on the secondary socket.
		 */

		len = sizeof(serv2_addr);
		sockfd2 = accept(socktemp, &serv2_addr, &len);
		close(socktemp);	/* done with this descriptor */
		if (sockfd2 < 0) {
			perror("accept");
			lport = 0;
			goto bad;
		}
		*fd2ptr = sockfd2;	/* to return to caller */

		/*
		 * The server has to bind its end of this connection to a
		 * reserved port also, or we don't accept it.
		 */

		serv2_addr.sin_port = ntohs((u_short) serv2_addr.sin_port);
		if ((serv2_addr.sin_family != AF_INET) ||
		    (serv2_addr.sin_port >= IPPORT_RESERVED) ||
		    (serv2_addr.sin_port <  IPPORT_RESERVED/2)) {
			fprintf(stderr,
			    "socket: protocol failure in circuit setup.\n");
			goto bad2;
		}
	}

	write(sockfd1, cliuname, strlen(cliuname)+1);
	write(sockfd1, servuname, strlen(servuname)+1);
	write(sockfd1, cmd, strlen(cmd)+1);

	if (read(sockfd1, &c, 1) != 1) {	/* read one byte from server */
		perror(*ahost);
		goto bad2;
	}

	if (c != 0) {
		/*
		 * We didn't get back the byte of zero.  There was an error
		 * detected by the server.  Read everything else on the
		 * socket up through a newline, which is an error message from
		 * the server, and copy to stderr.
		 */

		while (read(sockfd1, &c, 1) == 1) {
			write(2, &c, 1);
			if (c == '\n')
				break;
		}
		goto bad2;
	}

	sigsetmask(oldmask);
	return(sockfd1);	/* all OK, return socket descriptor */

bad2:
	if (lport)
		close(*fd2ptr);
	/* then fall through */
bad:
	close(sockfd1);
	sigsetmask(oldmask);
	return(-1);
}
