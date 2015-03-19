/*
 * Locking routines using semaphores.
 * Use the SEM_UNDO feature to have the kernel adjust the
 * semaphore value on premature exit.
 */

#include	<sys/types.h>
#include	<sys/ipc.h>
#include	<sys/sem.h>

#define	SEMKEY	123456L	/* key value for semget() */
#define	PERMS	0666

static struct sembuf	op_lock[2] = {
	0, 0, 0,	/* wait for sem#0 to become 0 */
	0, 1, SEM_UNDO	/* then increment sem#0 by 1 */
};

static struct sembuf	op_unlock[1] = {
	0, -1, (IPC_NOWAIT | SEM_UNDO)
			/* decrement sem#0 by 1 (sets it to 0) */
};

int	semid = -1;	/* semaphore id */

my_lock(fd)
int	fd;
{
	if (semid < 0) {
		if ( (semid = semget(SEMKEY, 1, IPC_CREAT | PERMS)) < 0)
			err_sys("semget error");
	}
	if (semop(semid, &op_lock[0], 2) < 0)
		err_sys("semop lock error");
}

my_unlock(fd)
int	fd;
{
	if (semop(semid, &op_unlock[0], 1) < 0)
		err_sys("semop unlock error");
}
