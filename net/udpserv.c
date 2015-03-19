/*
 * Example of server using UDP protocol.
 */

#include	"inet.h"

main(argc, argv)
int	argc;
char	*argv[];
{
	int			sockfd;
	struct sockaddr_in	serv_addr, cli_addr;

	pname = argv[0];

	/*
	 * Open a UDP socket (an Internet datagram socket).
	 */

	if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		err_dump("server: can't open datagram socket");

	/*
	 * Bind our local address so that the client can send to us.
	 */

	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family      = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port        = htons(SERV_UDP_PORT);

	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
		err_dump("server: can't bind local address");

	dg_echo(sockfd, (struct sockaddr *) &cli_addr, sizeof(cli_addr));

		/* NOTREACHED */
}
