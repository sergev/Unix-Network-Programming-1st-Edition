/*
 * Locking routines using a creat() system call with all
 * permissions turned off.
 */

#include	<sys/errno.h>
extern int	errno;

#define	LOCKFILE	"seqno.lock"
#define	TEMPLOCK	"temp.lock"

my_lock(fd)
int	fd;
{
	int	tempfd;

	/*
	 * Try to create a temporary file, with all write
	 * permissions turned off.  If the temporary file already
	 * exists, the creat() will fail.
	 */

	while ( (tempfd = creat(TEMPLOCK, 0)) < 0) {
		if (errno != EACCES)
			err_sys("creat error");
		sleep(1);
	}
	close(tempfd);
}

my_unlock(fd)
int	fd;
{
	if (unlink(TEMPLOCK) < 0)
		err_sys("unlink error for tempfile");
}
