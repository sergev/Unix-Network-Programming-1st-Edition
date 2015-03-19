/*
 * This function copies standard input to "fd" and also copies
 * everything from "fd" to standard output.
 * In addition, all this data that is copied could also
 * be recorded in a log file, if desired.
 * This is the 4.3BSD version that uses the select(2) system call.
 */

#include	<sys/types.h>
#include	<sys/time.h>

#define	BUFFSIZE	512

pass_all(fd, childpid)
int	fd;
int	childpid;
{
	int		maxfdp1, nfound, nread;
	char		buff[BUFFSIZE];
	fd_set		readmask;

	FD_ZERO(&readmask);

	for ( ; ; ) {
		FD_SET(0, &readmask);
		FD_SET(fd, &readmask);
		maxfdp1 = fd + 1;	/* check descriptors [0..fd] */

		nfound = select(maxfdp1, &readmask, (fd_set *) 0, (fd_set *) 0,
					(struct timeval *) 0);
		if (nfound < 0)
			err_sys("select error");

		if (FD_ISSET(0, &readmask)) {	/* data to read on stdin */
			nread = read(0, buff, BUFFSIZE);
			if (nread < 0)
				err_sys("read error from stdin");
			else if (nread == 0)
				break;		/* stdin EOF -> done */

			if (writen(fd, buff, nread) != nread)
				err_sys("writen error to stream pipe");
		}

		if (FD_ISSET(fd, &readmask)) {
					/* data to read on stream pipe */
			nread = read(fd, buff, BUFFSIZE);
			if (nread <= 0)
				break;		/* error or EOF, terminate */

			if (write(1, buff, nread) != nread)
				err_sys("write error to stdout");
		}
	}
}
