/*
 * Locking routines using the link() system call.
 */

#define	LOCKFILE	"seqno.lock"

#include	<sys/errno.h>
extern int	errno;

my_lock(fd)
int	fd;
{
	int	tempfd;
	char	tempfile[30];

	sprintf(tempfile, "LCK%d", getpid());

	/*
	 * Create a temporary file, then close it.
	 * If the temporary file already exists, the creat() will
	 * just truncate it to 0-length.
	 */

	if ( (tempfd = creat(tempfile, 0444)) < 0)
		err_sys("can't creat temp file");
	close(tempfd);

	/*
	 * Now try to rename the temporary file to the lock file.
	 * This will fail if the lock file already exists (i.e., if
	 * some other process already has a lock).
	 */

	while (link(tempfile, LOCKFILE) < 0) {
		if (errno != EEXIST)
			err_sys("link error");
		sleep(1);
	}
	if (unlink(tempfile) < 0)
		err_sys("unlink error for tempfile");
}

my_unlock(fd)
int	fd;
{
	if (unlink(LOCKFILE) < 0)
		err_sys("unlink error for LOCKFILE");
}
