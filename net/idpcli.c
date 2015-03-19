/*
 * Example of client using XNS IDP protocol.
 */

#include	"xns.h"

main(argc, argv)
int	argc;
char	*argv[];
{
	int			sockfd;
	struct sockaddr_ns	cli_addr, serv_addr;

	pname = argv[0];

	/*
	 * Fill in the structure "serv_addr" with the address of the
	 * server that we want to send to.
	 */

	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sns_family = AF_NS;
	serv_addr.sns_addr   = ns_addr(IDP_SERV_ADDR);
				/* stores net-ID, host-ID and port */

	/*
	 * Open an IDP socket (an XNS datagram socket).
	 */

	if ( (sockfd = socket(AF_NS, SOCK_DGRAM, 0)) < 0)
		err_dump("client: can't open datagram socket");

	/*
	 * Bind any local address for us.
	 */

	bzero((char *) &cli_addr, sizeof(cli_addr));	/* zero out */
	cli_addr.sns_family = AF_NS;
	if (bind(sockfd, (struct sockaddr *) &cli_addr, sizeof(cli_addr)) < 0)
		err_dump("client: can't bind local address");

	dg_cli(stdin, sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));

	close(sockfd);
	exit(0);
}
