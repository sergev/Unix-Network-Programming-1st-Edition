/*
 * Example of server using UNIX domain datagram protocol.
 */

#include	"unix.h"

main(argc, argv)
int	argc;
char	*argv[];
{
	int			sockfd, servlen;
	struct sockaddr_un	serv_addr, cli_addr;

	pname = argv[0];

	/*
	 * Open a socket (a UNIX domain datagram socket).
	 */

	if ( (sockfd = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0)
		err_dump("server: can't open datagram socket");

	/*
	 * Bind our local address so that the client can send to us.
	 */

	unlink(UNIXDG_PATH);	/* in case it was left from last time */
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sun_family = AF_UNIX;
	strcpy(serv_addr.sun_path, UNIXDG_PATH);
	servlen = sizeof(serv_addr.sun_family) + strlen(serv_addr.sun_path);

	if (bind(sockfd, (struct sockaddr *) &serv_addr, servlen) < 0)
		err_dump("server: can't bind local address");

	dg_echo(sockfd, (struct sockaddr *) &cli_addr, sizeof(cli_addr));
		/* NOTREACHED */
}
