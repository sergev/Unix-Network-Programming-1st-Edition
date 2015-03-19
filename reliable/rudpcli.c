/*
 * Example of a more reliable echo client using UDP protocol.
 */

#include	"rudp.h"

main(argc, argv)
int	argc;
char	*argv[];
{
	int	sockfd;

	pname = argv[0];

	if ( (sockfd = udp_open(HOST, MYECHO_SERVICE, 0, 0)) < 0)
		err_sys("udp_open error");

	doit(stdin, sockfd);		/* do it all */

	exit(0);
}

/*
 * Read the contents of the FILE *fp, write each line to the
 * socket (to the server process), then read a line back from
 * the socket and print it on the standard output.
 */

doit(fp, sockfd)
register FILE		*fp;
register int		sockfd;
{
	int	n, sendlen;
	long	seqsend;
	char	sendline[MAXLINE], recvline[MAXLINE];

	seqsend = 0;		/* initialize sequence number to send */
	while (fgets(sendline + sizeof(long), MAXLINE, fp) != NULL) {
		seqsend++;		/* increment sequence number */
		bcopy((char *) &seqsend, sendline, sizeof(long));
		sendlen = strlen(sendline + sizeof(long)) + sizeof(long);
rexmit:
		if ( (n = dgsendrecv(sockfd, sendline, sendlen, recvline,
									MAXLINE,
				(struct sockaddr *) 0, 0)) < 0) {
			if (errno == EINTR)
				err_sys("client: no response from server");
			else
				err_dump("client: dgsendrecv error");
		}

		if (bcmp((char *) &seqsend, recvline, sizeof(long)) != 0) {
			err_ret("incorrect sequence# received");
			goto rexmit;
		}

		recvline[n] = 0;		/* null terminate */
		fputs(recvline + sizeof(long), stdout);
	}

	if (ferror(fp))
		err_dump("client: error reading file");
}
