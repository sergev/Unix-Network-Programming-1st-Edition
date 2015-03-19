/*
 * Locking routines using a open() system call with both
 * O_CREAT and O_EXCL specified.
 */

#include	<fcntl.h>
#include	<sys/errno.h>
extern int	errno;

#define	LOCKFILE	"seqno.lock"
#define	PERMS		0666

my_lock(fd)
int	fd;
{
	int	tempfd;

	/*
	 * Try to create the lock file, using open() with both
	 * O_CREAT (create file if it doesn't exist) and O_EXCL
	 * (error if create and file already exists).
	 * If this fails, some other process has the lock.
	 */

	while ( (tempfd = open(LOCKFILE, O_RDWR|O_CREAT|O_EXCL, PERMS)) < 0) {
		if (errno != EEXIST)
			err_sys("open error for lock file");
		sleep(1);
	}
	close(tempfd);
}

my_unlock(fd)
int	fd;
{
	if (unlink(LOCKFILE) < 0)
		err_sys("unlink error for lock file");
}
