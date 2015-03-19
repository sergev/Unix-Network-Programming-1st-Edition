/*
 * Test the System V Release 3 advisory versus mandatory lock.
 */

#include	<unistd.h>
#include	<fcntl.h>
#include	<sys/errno.h>
extern int	errno;

#define	TEMPFILE	"temp.foo"

main()
{
	int	fd, pid;
	char	buff[5];

	if ( (pid = fork()) < 0) {
		err_sys("fork error");

	} else if (pid > 0) {			/* parent */
		if ( (fd = open(TEMPFILE, 2)) < 0)
			err_sys("parent: open error");
		if (lockf(fd, F_TLOCK, 0L) < 0)
			err_sys("parent: F_TLOCK error");

		while (wait((int *) 0) != pid)
			;

		close(fd);
		exit(0);

	} else {			/* child */
		sleep(1);
		if ( (fd = open(TEMPFILE, 0)) < 0)
			err_sys("child: open error");
		if (fcntl(fd, F_SETFL, O_NDELAY) < 0)
			err_sys("child: fcntl error");

		if (lockf(fd, F_TEST, 0L) == 0)
			err_sys("child: F_TEST succeeded");
		if ((errno != EACCES) && (errno != EAGAIN))
			err_sys("child: bad errno after F_TEST");

		if (read(fd, buff, 5) < 0)
			printf("child: read failed, errno = %d\n", errno);
		else
			printf("child: read OK, buff = %5.5s\n", buff);

		close(fd);
		exit(0);
	}
}
