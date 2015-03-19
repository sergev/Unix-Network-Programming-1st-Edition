/*
 * Example of server using TCP protocol.
 */

#include	"inet.h"

main(argc, argv)
int	argc;
char	*argv[];
{
	int			sockfd, newsockfd, clilen, childpid;
	struct sockaddr_in	cli_addr, serv_addr;

	pname = argv[0];

	/*
	 * Open a TCP socket (an Internet stream socket).
	 */

	if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		err_dump("server: can't open stream socket");

	/*
	 * Bind our local address so that the client can send to us.
	 */

	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family      = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port        = htons(SERV_TCP_PORT);

	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
		err_dump("server: can't bind local address");

	listen(sockfd, 5);

	for ( ; ; ) {
		/*
		 * Wait for a connection from a client process.
		 * This is an example of a concurrent server.
		 */

		clilen = sizeof(cli_addr);
		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr,
								     &clilen);
		if (newsockfd < 0)
			err_dump("server: accept error");

		if ( (childpid = fork()) < 0)
			err_dump("server: fork error");

		else if (childpid == 0) {	/* child process */
			close(sockfd);		/* close original socket */
			str_echo(newsockfd);	/* process the request */
			exit(0);
		}

		close(newsockfd);		/* parent process */
	}
}
