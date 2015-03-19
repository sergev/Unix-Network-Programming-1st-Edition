/*
 * Example of client using TCP protocol.
 */

#include	"inet.h"

main(argc, argv)
int	argc;
char	*argv[];
{
	int			tfd;
	char			*t_alloc();	/* TLI function */
	struct t_call		*callptr;
	struct sockaddr_in	serv_addr;

	pname = argv[0];

	/*
	 * Create a TCP transport endpoint and bind it.
	 */

	if ( (tfd = t_open(DEV_TCP, O_RDWR, 0)) < 0)
		err_sys("client: can't t_open %s", DEV_TCP);

	if (t_bind(tfd, (struct t_bind *) 0, (struct t_bind *) 0) < 0)
		err_sys("client: t_bind error");

	/*
	 * Fill in the structure "serv_addr" with the address of the
	 * server that we want to connect with.
	 */

	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family      = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(SERV_HOST_ADDR);
	serv_addr.sin_port        = htons(SERV_TCP_PORT);

	/*
	 * Allocate a t_call structure, and initialize it.
	 * Let t_alloc() initialize the addr structure of the t_call structure.
	 */

	if ( (callptr = (struct t_call *) t_alloc(tfd, T_CALL, T_ADDR)) == NULL)
		err_sys("client: t_alloc error");
	callptr->addr.maxlen = sizeof(serv_addr);
	callptr->addr.len    = sizeof(serv_addr);
	callptr->addr.buf    = (char *) &serv_addr;
	callptr->opt.len     = 0;		/* no options */
	callptr->udata.len   = 0;		/* no user data with connect */

	/*
	 * Connect to the server.
	 */

	if (t_connect(tfd, callptr, (struct t_call *) 0) < 0)
		err_sys("client: can't t_connect to server");

	doit(stdin, tfd);	/* do it all */

	close(tfd);
	exit(0);
}

/*
 * Read the contents of the FILE *fp, write each line to the
 * transport endpoint (to the server process), then read a line back from
 * the transport endpoint and print it on the standard output.
 */

doit(fp, tfd)
register FILE	*fp;
register int	tfd;
{
	int	n, flags;
	char	sendline[MAXLINE], recvline[MAXLINE + 1];

	while (fgets(sendline, MAXLINE, fp) != NULL) {
		n = strlen(sendline);
		if (t_snd(tfd, sendline, n, 0) != n)
			err_sys("client: t_snd error");

		/*
		 * Now read a line from the transport endpoint and write it to
		 * our standard output.
		 */

		n = t_rcv(tfd, recvline, MAXLINE, &flags);
		if (n < 0)
			err_dump("client: t_rcv error");
		recvline[n] = 0;	/* null terminate */
		fputs(recvline, stdout);
	}

	if (ferror(fp))
		err_sys("client: error reading file");
}
