/*
 * Example of client using UNIX domain stream protocol.
 */

#include	"unix.h"

main(argc, argv)
int	argc;
char	*argv[];
{
	int			sockfd, servlen;
	struct sockaddr_un	serv_addr;

	pname = argv[0];

	/*
	 * Fill in the structure "serv_addr" with the address of the
	 * server that we want to send to.
	 */

	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sun_family = AF_UNIX;
	strcpy(serv_addr.sun_path, UNIXSTR_PATH);
	servlen = strlen(serv_addr.sun_path) + sizeof(serv_addr.sun_family);

	/*
	 * Open a socket (an UNIX domain stream socket).
	 */

	if ( (sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
		err_sys("client: can't open stream socket");

	/*
	 * Connect to the server.
	 */

	if (connect(sockfd, (struct sockaddr *) &serv_addr, servlen) < 0)
		err_sys("client: can't connect to server");

	str_cli(stdin, sockfd);		/* do it all */

	close(sockfd);
	exit(0);
}
