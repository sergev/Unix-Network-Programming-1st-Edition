/*
 * Example of getsockopt() and setsockopt().
 */

#include	<sys/types.h>
#include	<sys/socket.h>		/* for SOL_SOCKET and SO_xx values */
#include	<netinet/in.h>		/* for IPPROTO_TCP value */
#include	<netinet/tcp.h>		/* for TCP_MAXSEG value */

main()
{
	int	sockfd, maxseg, sendbuff, optlen;

	if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		err_sys("can't create socket");

	/*
	 * Fetch and print the TCP maximum segment size.
	 */

	optlen = sizeof(maxseg);
	if (getsockopt(sockfd, IPPROTO_TCP, TCP_MAXSEG, (char *) &maxseg,
				&optlen) < 0)
		err_sys("TCP_MAXSEG getsockopt error");
	printf("TCP maxseg = %d\n", maxseg);

	/*
	 * Set the send buffer size, then fetch it and print its value.
	 */

	sendbuff = 16384;	/* just some number for example purposes */
	if (setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, (char *) &sendbuff,
							sizeof(sendbuff)) < 0)
		err_sys("SO_SNDBUF setsockopt error");

	optlen = sizeof(sendbuff);
	if (getsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, (char *) &sendbuff,
							&optlen) < 0)
		err_sys("SO_SNDBUF getsockopt error");
	printf("send buffer size = %d\n", sendbuff);
}
