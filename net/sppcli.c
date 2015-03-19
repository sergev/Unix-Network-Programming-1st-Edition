/*
 * Example of client using SPP protocol.
 */

#include	"xns.h"

main(argc, argv)
int	argc;
char	*argv[];
{
	int			sockfd;
	struct sockaddr_ns	serv_addr;

	pname = argv[0];

	/*
	 * Fill in the structure "serv_addr" with the XNS address of the
	 * server that we want to connect with.
	 */

	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sns_family = AF_NS;
	serv_addr.sns_addr   = ns_addr(SPP_SERV_ADDR);
				/* stores net-ID, host-ID and port */

	/*
	 * Open a SPP socket (an XNS stream socket).
	 */

	if ( (sockfd = socket(AF_NS, SOCK_STREAM, 0)) < 0)
		err_sys("client: can't open stream socket");

	/*
	 * Connect to the server.
	 */

	if (connect(sockfd, (struct sockaddr *) &serv_addr,
							sizeof(serv_addr)) < 0)
		err_sys("client: can't connect to server");

	str_cli(stdin, sockfd);		/* do it all */

	close(sockfd);
	exit(0);
}
