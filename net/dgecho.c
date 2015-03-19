/*
 * Read a datagram from a connectionless socket and write it back to
 * the sender.
 *
 * We never return, as we never know when a datagram client is done.
 */

#include	<sys/types.h>
#include	<sys/socket.h>

#define	MAXMESG	2048

dg_echo(sockfd, pcli_addr, maxclilen)
int		sockfd;
struct sockaddr	*pcli_addr;	/* ptr to appropriate sockaddr_XX structure */
int		maxclilen;	/* sizeof(*pcli_addr) */
{
	int	n, clilen;
	char	mesg[MAXMESG];

	for ( ; ; ) {
		clilen = maxclilen;
		n = recvfrom(sockfd, mesg, MAXMESG, 0, pcli_addr, &clilen);
		if (n < 0)
			err_dump("dg_echo: recvfrom error");

		if (sendto(sockfd, mesg, n, 0, pcli_addr, clilen) != n)
			err_dump("dg_echo: sendto error");
	}
}
