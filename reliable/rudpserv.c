/*
 * Example of server using UDP protocol.
 * This server has a run-time option to discard packets and to
 * modify the sequence number, to allow testing of the more
 * reliable client side.
 *
 *	rudpserv [ -e <delay> ] [ -d <percent> ] [ -s <percent> ]
 */

#include	"rudp.h"
#include	<netdb.h>

int	discardrate = 0;	/* should be [0-100] */
int	delay = 0;		/* delay in responding, seconds */
int	seqmodrate  = 0;	/* should be [0-100] */

main(argc, argv)
int	argc;
char	*argv[];
{
	int			sockfd;
	char			*s;
	struct sockaddr_in	serv_addr;
	struct servent		*sp;

	pname = argv[0];
	while (--argc > 0 && (*++argv)[0] == '-')
		for (s = argv[0]+1; *s != '\0'; s++)
			switch (*s) {

			case 'd':	/* next arg is discard rate */
				if (--argc <=0)
				   err_quit("-d requires another argument");
				discardrate = atoi(*++argv);
				break;

			case 'e':	/* next arg is delay in seconds */
				if (--argc <=0)
				   err_quit("-e requires another argument");
				delay = atoi(*++argv);
				break;

			case 's':	/* next arg is sequence mod rate */
				if (--argc <=0)
				   err_quit("-s requires another argument");
				seqmodrate = atoi(*++argv);
				break;

			default:
				err_quit("illegal option %c", *s);
			}

	/*
	 * Open a UDP socket (an Internet datagram socket).
	 */

	if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		err_dump("server: can't open datagram socket");

	/*
	 * Find out what port we should be listening on, then bind
	 * our local address so that the client can send to us.
	 */

	if ( (sp = getservbyname(MYECHO_SERVICE, "udp")) == NULL)
		err_quit("server: unknown service: %s/udp", MYECHO_SERVICE);

	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family      = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port        = sp->s_port;

	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
		err_dump("server: can't bind local address");

	echo(sockfd);		/* do it all */
		/* NOTREACHED */
}

/*
 * Read the contents of the socket and write each line back to
 * the sender.
 */

echo(sockfd)
int	sockfd;
{
	int			n, clilen;
	long			percent, random(), time();
	char			line[MAXLINE];
	struct sockaddr_in	cli_addr;

	if (discardrate || seqmodrate)	/* init random number sequence */
		srandom( (int) time((long *) 0) );

	for ( ; ; ) {
		/*
		 * Read a message from the socket and send it back
		 * to whomever sent it.
		 */

ignore:
		clilen = sizeof(cli_addr);
		n = recvfrom(sockfd, line, MAXLINE, 0,
				(struct sockaddr *) &cli_addr, &clilen);
		if (n < 0)
			err_dump("server: recvfrom error");

		/*
		 * First see if we should delay before doing anything.
		 */

		if (delay)
			sleep(delay);

		/*
		 * See if we should discard this packet.
		 */

		if (discardrate) {
			percent = (random() % 100) + 1;		/* [1, 100] */
			if (percent <= discardrate)
				goto ignore;
		}

		/*
		 * See if we should modify the sequence number of
		 * this packet.
		 */

		if (seqmodrate) {
			percent = (random() % 100) + 1;		/* [1, 100] */
			if (percent <= seqmodrate)
				line[percent & 3]++;
				/* change one of line[0], [1], [2] or [3] */
		}

		if (sendto(sockfd, line, n, 0, (struct sockaddr *)
				&cli_addr, clilen) != n)
			err_dump("server: sendto error");
	}
}
