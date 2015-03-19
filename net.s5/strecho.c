/*
 * Read a stream socket one line at a time, and write each line back
 * to the sender.
 *
 * Return when the connection is terminated.
 */

#define	MAXLINE	512

str_echo(sockfd)
int	sockfd;
{
	int	n;
	char	line[MAXLINE];

	for ( ; ; ) {
		n = readline(sockfd, line, MAXLINE);
		if (n == 0)
			return;		/* connection terminated */
		else if (n < 0)
			err_dump("str_echo: readline error");

		if (writen(sockfd, line, n) != n)
			err_dump("str_echo: writen error");
	}
}
