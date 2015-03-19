/*
 * Example of server using XNS IDP protocol.
 */

#include	"xns.h"

main(argc, argv)
int	argc;
char	*argv[];
{
	int			sockfd;
	struct sockaddr_ns	serv_addr, cli_addr;

	pname = argv[0];

	/*
	 * Open an IDP socket (an XNS datagram socket).
	 */

	if ( (sockfd = socket(AF_NS, SOCK_DGRAM, 0)) < 0)
		err_dump("server: can't open datagram socket");

	/*
	 * Bind our local address so that the client can send to us.
	 */

	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sns_family       = AF_NS;
	serv_addr.sns_addr.x_port  = htons(SERV_IDP_PORT);

	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
		err_dump("server: can't bind local address");

	dg_echo(sockfd, (struct sockaddr *) &cli_addr, sizeof(cli_addr));
		/* NOTREACHED */
}
