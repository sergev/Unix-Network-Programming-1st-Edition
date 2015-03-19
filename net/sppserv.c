/*
 * Example of server using SPP protocol.
 */

#include	"xns.h"

main(argc, argv)
int	argc;
char	*argv[];
{
	int			sockfd, newsockfd, clilen;
	struct sockaddr_ns	cli_addr, serv_addr;

	pname = argv[0];

	/*
	 * Open a SPP socket (an XNS stream socket).
	 */

	if ( (sockfd = socket(AF_NS, SOCK_STREAM, 0)) < 0)
		err_dump("server: can't open stream socket");

	/*
	 * Bind our local address so that the client can send to us.
	 */

	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sns_family      = AF_NS;
	serv_addr.sns_addr.x_port = htons(SERV_SPP_PORT);

	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
		err_dump("server: can't bind local address");

	listen(sockfd, 5);

	for ( ; ; ) {
		/*
		 * Wait for a connection from a client process,
		 * then process it without fork()'ing.
		 * This is an example of an iterative server.
		 */

		clilen = sizeof(cli_addr);
		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr,
								     &clilen);
		if (newsockfd < 0)
			err_dump("server: accept error");

		str_echo(newsockfd);	/* returns when connection is closed */

		close(newsockfd);
	}
}
