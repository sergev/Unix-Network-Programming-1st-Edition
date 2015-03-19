/*
 * This is an example of an XNS echo client.
 * It uses the protocol defined in the Xerox manual "Internet Transport
 * Protocols" (XNSS 028112, Dec. 1981), Chapter 6, pp. 37-38.
 * Notice that on 4.3BSD systems, the kernel handles these requests,
 * if a server isn't listening on the echo port.
 */

#include	<stdio.h>
#include	<sys/types.h>
#include	<sys/socket.h>
#include	<netns/ns.h>
#include	<netns/idp.h>

#define	ECHO_SERV_ADDR	"123:02.07.01.00.6d.82:2"
				/* <netid>:<hostid>:<port> */
				/* the <port> of 2 is the well-known port
				   (XNS "socket") for an XNS echo server */

#define	ECHO_REQUEST	1	/* client sends a request as operation field */
#define	ECHO_REPLY	2	/* server responds with this as op field */
#define	OP_SIZE		sizeof(short)
			/* size of the operation field at front of packet */

#define	MAXLINE		255

extern int	rtt_d_flag;	/* defined in the rtt.h header file */
				/* to print the RTT timing info */
char	*pname;

main(argc, argv)
int	argc;
char	*argv[];
{
	FILE			*fp, *fopen();
	register int		i, sock;
	struct idp		idp_hdr;

	pname = argv[0];
	--argc; ++argv;

	rtt_d_flag = 1;		/* to print the RTT timing info */

	/*
	 * Create an IDP socket, bind any local address and record the
	 * server's address.
	 */

	if ( (sock = idp_open(ECHO_SERV_ADDR, (char *) 0, 0)) < 0)
		err_sys("can't create IDP socket");

	/*
	 * Set the socket option for default headers on output.
	 * We set the packet type field of the IDP header to NSPROTO_ECHO.
	 * Note that all the system uses from this structure is the packet
	 * type field (idp_pt), and this will be the packet type on all
	 * datagrams sent on this socket.
	 * Note that the packet type field is a single byte, so there are
	 * no byte-ordering problems.
	 */

	idp_hdr.idp_pt = NSPROTO_ECHO;		/* packet type */
	if (setsockopt(sock, 0, SO_DEFAULT_HEADERS,
			(char *) &idp_hdr, sizeof(idp_hdr)) < 0)
		err_sys("setsockopt error");

	/*
	 * Main loop.
	 * For every command line argument (or stdin) call doit();
	 */

	i = 0;
	fp = stdin;
	do {
		if (argc > 0 && (fp = fopen(argv[i], "r")) == NULL) {
			fprintf(stderr, "%s: can't open %s\n", pname, argv[i]);
			continue;
		}

		doit(sock, fp);

	} while (++i < argc);

	close(sock);
	exit(0);
}

/*
 * Read the contents of the FILE *fp, write each line to the
 * socket (to the server process), then read a line back from
 * the socket and print it on the standard output.
 */

doit(sock, fp)
register int	sock;
register FILE	*fp;
{
	int	n, sendlen;
	char	*fgets();
	char	sendline[MAXLINE], recvline[MAXLINE];

	while (fgets(sendline + OP_SIZE, MAXLINE, fp) != NULL) {
		/*
		 * Set the first 2 bytes of the packet to ECHO_REQUEST.
		 */

		*( u_short * ) sendline = htons(ECHO_REQUEST);
						/* op = echo request */
		sendlen = strlen(sendline + OP_SIZE) + OP_SIZE;

		if ( (n = dgsendrecv(sock, sendline, sendlen, recvline, MAXLINE,
				(struct sockaddr *) 0, 0)) < 0)
			err_sys("drsendrecv error");

		/*
		 * There had better be at least 2 bytes in the datagram, and
		 * the first 2 bytes must be ECHO_REPLY.
		 */

		if (n < OP_SIZE)
			err_dump("invalid length");
		if ( (*(u_short *) recvline) != htons(ECHO_REPLY))
			err_dump("unexpected operation field");

		recvline[n] = 0;	/* null terminate */
		fputs(recvline + OP_SIZE, stdout);
	}

	if (ferror(fp))
		err_sys("error reading file");

	fclose(fp);	/* close the FILE, leave the socket open */
}
