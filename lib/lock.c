/*
 * Locking routines for 4.3BSD.
 */

#include	<sys/file.h>

my_lock(fd)
int	fd;
{
	if (flock(fd, LOCK_EX) == -1)
		err_sys("can't LOCK_EX");
}

my_unlock(fd)
int	fd;
{
	if (flock(fd, LOCK_UN) == -1)
		err_sys("can't LOCK_UN");
}
