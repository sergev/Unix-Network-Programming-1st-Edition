/*
 * Locking routines for System V.
 */

#include	<unistd.h>

my_lock(fd)
int	fd;
{
	lseek(fd, 0L, 0);			/* rewind before lockf */
	if (lockf(fd, F_LOCK, 0L) == -1)	/* 0L -> lock entire file */
		err_sys("can't F_LOCK");
}

my_unlock(fd)
int	fd;
{
	lseek(fd, 0L, 0);
	if (lockf(fd, F_ULOCK, 0L) == -1)
		err_sys("can't F_ULOCK");
}
