#include	<stdio.h>

#define	MAXBUFF		1024

client(readfd, writefd)
int	readfd;
int	writefd;
{
	char	buff[MAXBUFF];
	int	n;

	/*
	 * Read the filename from standard input,
	 * write it to the IPC descriptor.
	 */

	if (fgets(buff, MAXBUFF, stdin) == NULL)
		err_sys("client: filename read error");

	n = strlen(buff);
	if (buff[n-1] == '\n')
		n--;			/* ignore newline from fgets() */
	if (write(writefd, buff, n) != n)
		err_sys("client: filename write error");;

	/*
	 * Read the data from the IPC descriptor and write
	 * to standard output.
	 */

	while ( (n = read(readfd, buff, MAXBUFF)) > 0)
		if (write(1, buff, n) != n)	/* fd 1 = stdout */
			err_sys("client: data write error");
	if (n < 0)
		err_sys("client: data read error");
}
