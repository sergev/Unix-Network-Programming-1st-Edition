/*
 * Example of server using UDP protocol.
 */

#include	"inet.h"

main(argc, argv)
int	argc;
char	*argv[];
{
	int			tfd;
	struct sockaddr_in	serv_addr;
	struct t_bind		req;

	pname = argv[0];

	/*
	 * Open a UDP endpoint.
	 */

	if ( (tfd = t_open(DEV_UDP, O_RDWR, (struct t_info *) 0)) < 0)
		err_dump("server: can't t_open %s", DEV_UDP);

	/*
	 * Bind our local address so that the client can send to us.
	 */

	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family      = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port        = htons(SERV_UDP_PORT);

	req.addr.maxlen = sizeof(serv_addr);
	req.addr.len    = sizeof(serv_addr);
	req.addr.buf    = (char *) &serv_addr;
	req.qlen        = 5;

	if (t_bind(tfd, &req, (struct t_bind *) 0) < 0)
		err_dump("server: can't t_bind local address");

	echo(tfd);		/* do it all */
		/* NOTREACHED */
}

/*
 * Read the contents of the socket and write each line back to
 * the sender.
 */

echo(tfd)
int	tfd;
{
	int			n, flags;
	char			line[MAXLINE];
	char			*t_alloc();
	struct t_unitdata	*udataptr;

	/*
	 * Allocate memory for the t_unitdata structure and the address field
	 * in that structure.  This allows any size of address to be handled
	 * by this function.
	 */

	udataptr = (struct t_unitdata *) t_alloc(tfd, T_UNITDATA, T_ADDR);
	if (udataptr == NULL)
		err_dump("server: t_alloc error for T_UNITDATA");

	for ( ; ; ) {
		/*
		 * Read a message from the socket and send it back
		 * to whomever sent it.
		 */

		udataptr->opt.maxlen   = 0;	/* don't care about options */
		udataptr->opt.len      = 0;
		udataptr->udata.maxlen = MAXLINE;
		udataptr->udata.len    = MAXLINE;
		udataptr->udata.buf    = line;
		if (t_rcvudata(tfd, udataptr, &flags) < 0)
			err_dump("server: t_rcvudata error");

		if (t_sndudata(tfd, udataptr) < 0)
			err_dump("server: t_sndudata error");
	}
}
