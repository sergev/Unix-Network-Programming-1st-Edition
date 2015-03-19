/*
 * Example of client using UDP protocol.
 */

#include	"inet.h"

main(argc, argv)
int	argc;
char	*argv[];
{
	int			sockfd;
	struct sockaddr_in	cli_addr, serv_addr;

	pname = argv[0];

	/*
	 * Fill in the structure "serv_addr" with the address of the
	 * server that we want to send to.
	 */

	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family      = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(SERV_HOST_ADDR);
	serv_addr.sin_port        = htons(SERV_UDP_PORT);

	/*
	 * Open a UDP socket (an Internet datagram socket).
	 */

	if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		err_dump("client: can't open datagram socket");

	/*
	 * Bind any local address for us.
	 */

	bzero((char *) &cli_addr, sizeof(cli_addr));	/* zero out */
	cli_addr.sin_family      = AF_INET;
	cli_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	cli_addr.sin_port        = htons(0);
	if (bind(sockfd, (struct sockaddr *) &cli_addr, sizeof(cli_addr)) < 0)
		err_dump("client: can't bind local address");

	dg_cli(stdin, sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));

	close(sockfd);
	exit(0);
}
