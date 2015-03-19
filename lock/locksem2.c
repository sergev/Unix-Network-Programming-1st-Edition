/*
 * Locking routines using simpler semaphore routines.
 */

#include	<sys/types.h>
#include	<sys/ipc.h>
#include	<sys/sem.h>

#define	SEMKEY	123456L	/* key value for sem_create() */

int	semid = -1;	/* semaphore id */

my_lock(fd)
int	fd;
{
	if (semid < 0) {
		if ( (semid = sem_create(SEMKEY, 1)) < 0)
			err_sys("sem_create error");
	}
	sem_wait(semid);
}

my_unlock(fd)
int	fd;
{
	sem_signal(semid);
}
