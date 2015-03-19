/*
 * Example of server using TCP protocol.
 */

#include	"inet.h"

main(argc, argv)
int	argc;
char	*argv[];
{
	int			tfd, newtfd, clilen, childpid;
	struct sockaddr_in	cli_addr, serv_addr;
	struct t_bind		req;
	struct t_call		*callptr;

	pname = argv[0];

	/*
	 * Create a TCP transport endpoint.
	 */

	if ( (tfd = t_open(DEV_TCP, O_RDWR, (struct t_info *) 0)) < 0)
		err_dump("server: can't t_open %s", DEV_TCP);

	/*
	 * Bind our local address so that the client can send to us.
	 */

	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family      = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port        = htons(SERV_TCP_PORT);

	req.addr.maxlen = sizeof(serv_addr);
	req.addr.len    = sizeof(serv_addr);
	req.addr.buf    = (char *) &serv_addr;
	req.qlen        = 5;

	if (t_bind(tfd, &req, (struct t_bind *) 0) < 0)
		err_dump("server: can't t_bind local address");

	/*
	 * Allocate a t_call structure for t_listen() and t_accept().
	 */

	if ( (callptr = (struct t_call *) t_alloc(tfd, T_CALL, T_ADDR)) == NULL)
		err_dump("server: t_alloc error for T_CALL");

	for ( ; ; ) {
		/*
		 * Wait for a connection from a client process.
		 * This is an example of a concurrent server.
		 */

		if (t_listen(tfd, callptr) < 0)
			err_dump("server: t_listen error");

		if ( (newtfd = accept_call(tfd, callptr, DEV_TCP, 1)) < 0)
			err_dump("server: accept_call error");

		if ( (childpid = fork()) < 0)
			err_dump("server: fork error");

		else if (childpid == 0) {	/* child process */
			t_close(tfd);		/* close original endpoint */
			str_echo(newtfd);	/* process the request */
			exit(0);
		}

		close(newtfd);		/* parent process */
	}
}
