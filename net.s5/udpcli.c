/*
 * Example of client using UDP protocol.
 */

#include	"inet.h"

main(argc, argv)
int	argc;
char	*argv[];
{
	int			tfd;
	struct t_unitdata	unitdata;
	struct sockaddr_in	serv_addr;

	pname = argv[0];

	/*
	 * Open a UDP endpoint.
	 */

	if ( (tfd = t_open(DEV_UDP, O_RDWR, (struct t_info *) 0)) < 0)
		err_dump("client: can't t_open %s", DEV_UDP);

	/*
	 * Bind any local address for us.
	 */

	if (t_bind(tfd, (struct t_bind *) 0, (struct t_bind *) 0) < 0)
		err_dump("client: t_bind error");

	/*
	 * Initialize a sockaddr_in structure with the address of the
	 * the server we want to send datagrams to.
	 */

	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family      = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(SERV_HOST_ADDR);
	serv_addr.sin_port        = htons(SERV_UDP_PORT);

	/*
	 * Now initialize a unitdata structure for sending to the server.
	 */

	unitdata.addr.maxlen = sizeof(serv_addr);	/* server's addr */
	unitdata.addr.len    = sizeof(serv_addr);
	unitdata.addr.buf    = (char *) &serv_addr;
	unitdata.opt.maxlen  = 0;			/* no options */
	unitdata.opt.len     = 0;
	unitdata.opt.buf     = (char *) 0;

	doit(tfd, &unitdata, stdin);	/* do it all */

	t_close(tfd);
	exit(0);
}

/*
 * Read the contents of the FILE *fp, write each line to the transport
 * endpoint (to the server process), then read a line back from
 * the transport endpoint and print it on the standard output.
 */

doit(tfd, sudataptr, fp)
register int		tfd;
struct t_unitdata	*sudataptr;	/* unitdata for sends */
register FILE		*fp;
{
	int			n, flags;
	char			sendline[MAXLINE], recvline[MAXLINE + 1];
	char			*t_alloc();
	struct t_unitdata	*rudataptr;	/* unitdata for receives */

	/*
	 * Allocate memory for the t_unitdata structure and the address field
	 * in that structure.  This allows any size of address to be handled
	 * by this function.
	 */

	rudataptr = (struct t_unitdata *) t_alloc(tfd, T_UNITDATA, T_ADDR);
	if (rudataptr == NULL)
		err_dump("server: t_alloc error for T_UNITDATA");

	while (fgets(sendline, MAXLINE, fp) != NULL) {
		n = strlen(sendline);
		sudataptr->udata.len = n;
		sudataptr->udata.buf = sendline;
		if (t_sndudata(tfd, sudataptr) < 0)
			err_dump("client: t_sndudata error");

		/*
		 * Now read a message from the transport endpoint and
		 * write it to our standard output.
		 */

		rudataptr->opt.maxlen = 0;	/* don't care about options */
		rudataptr->udata.maxlen = MAXLINE;
		rudataptr->udata.buf    = recvline;
		if (t_rcvudata(tfd, rudataptr, &flags) < 0)
			err_dump("client: t_rcvudata error");
		recvline[rudataptr->udata.len] = 0;	/* null terminate */
		fputs(recvline, stdout);
	}

	if (ferror(fp))
		err_dump("client: error reading file");
}
