/*
 * Example of client using UNIX domain datagram protocol.
 */

#include	"unix.h"

main(argc, argv)
int	argc;
char	*argv[];
{
	int			sockfd, clilen, servlen;
	char			*mktemp();
	struct sockaddr_un	cli_addr, serv_addr;

	pname = argv[0];

	/*
	 * Fill in the structure "serv_addr" with the address of the
	 * server that we want to send to.
	 */

	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sun_family = AF_UNIX;
	strcpy(serv_addr.sun_path, UNIXDG_PATH);
	servlen = sizeof(serv_addr.sun_family) + strlen(serv_addr.sun_path);

	/*
	 * Open a socket (a UNIX domain datagram socket).
	 */

	if ( (sockfd = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0)
		err_dump("client: can't open datagram socket");

	/*
	 * Bind a local address for us.
	 * In the UNIX domain we have to choose our own name (that
	 * should be unique).  We'll use mktemp() to create a unique
	 * pathname, based on our process id.
	 */

	bzero((char *) &cli_addr, sizeof(cli_addr));	/* zero out */
	cli_addr.sun_family = AF_UNIX;
	strcpy(cli_addr.sun_path, UNIXDG_TMP);
	mktemp(cli_addr.sun_path);
	clilen = sizeof(cli_addr.sun_family) + strlen(cli_addr.sun_path);

	if (bind(sockfd, (struct sockaddr *) &cli_addr, clilen) < 0)
		err_dump("client: can't bind local address");

	dg_cli(stdin, sockfd, (struct sockaddr *) &serv_addr, servlen);

	close(sockfd);
	unlink(cli_addr.sun_path);
	exit(0);
}
