#include	<sys/types.h>
#include	<sys/ipc.h>
#include	<sys/sem.h>

#define	KEY	((key_t) 54321)
#define	COUNT	20000

struct sembuf	op_up[1]   = { 0,  1, 0 };
struct sembuf	op_down[1] = { 0, -1, 0 };

main()
{
	register int	i, semid;

	if ( (semid = semget(KEY, 1, 0666 | IPC_CREAT)) < 0)
		err_sys("semget error");

	for (i = 0; i < COUNT; i++) {
		if (semop(semid, &op_up[0], 1) < 0)
			err_sys("semop up error");

		if (semop(semid, &op_down[0], 1) < 0)
			err_sys("semop down error");
	}

	if (semctl(semid, 0, IPC_RMID, (struct semid_ds *) 0) < 0)
		err_sys("IPC_RMID error");

	exit(0);
}
