#include	<stdio.h>

#define	MAXBUFF		1024

server(readfd, writefd)
int	readfd;
int	writefd;
{
	char		buff[MAXBUFF];
	char		errmesg[256], *sys_err_str();
	int		n, fd;
	extern int	errno;

	/*
	 * Read the filename from the IPC descriptor.
	 */

	if ( (n = read(readfd, buff, MAXBUFF)) <= 0)
		err_sys("server: filename read error");
	buff[n] = '\0';		/* null terminate filename */

	if ( (fd = open(buff, 0)) < 0) {
		/*
		 * Error.  Format an error message and send it back
		 * to the client.
		 */

		sprintf(errmesg, ": can't open, %s\n", sys_err_str());
		strcat(buff, errmesg);
		n = strlen(buff);
		if (write(writefd, buff, n) != n)
			err_sys("server: errmesg write error");
	} else {
		/*
		 * Read the data from the file and write to
		 * the IPC descriptor.
		 */

		while ( (n = read(fd, buff, MAXBUFF)) > 0)
			if (write(writefd, buff, n) != n)
				err_sys("server: data write error");
		if (n < 0)
			err_sys("server: read error");
	}
}
