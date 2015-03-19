/*
 * Read the contents of the FILE *fp, write each line to the
 * datagram socket, then read a line back from the datagram
 * socket and write it to the standard output.
 *
 * Return to caller when an EOF is encountered on the input file.
 */

#include	<stdio.h>
#include	<sys/types.h>
#include	<sys/socket.h>

#define	MAXLINE	512

dg_cli(fp, sockfd, pserv_addr, servlen)
FILE		*fp;
int		sockfd;
struct sockaddr	*pserv_addr;	/* ptr to appropriate sockaddr_XX structure */
int		servlen;	/* actual sizeof(*pserv_addr) */
{
	int	n;
	char	sendline[MAXLINE], recvline[MAXLINE + 1];

	while (fgets(sendline, MAXLINE, fp) != NULL) {
		n = strlen(sendline);
		if (sendto(sockfd, sendline, n, 0, pserv_addr, servlen) != n)
			err_dump("dg_cli: sendto error on socket");

		/*
		 * Now read a message from the socket and write it to
		 * our standard output.
		 */

		n = recvfrom(sockfd, recvline, MAXLINE, 0,
				(struct sockaddr *) 0, (int *) 0);
		if (n < 0)
			err_dump("dg_cli: recvfrom error");
		recvline[n] = 0;	/* null terminate */
		fputs(recvline, stdout);
	}

	if (ferror(fp))
		err_dump("dg_cli: error reading file");
}
