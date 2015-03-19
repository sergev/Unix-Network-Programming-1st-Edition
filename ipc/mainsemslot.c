#include	<sys/types.h>
#include	<sys/ipc.h>
#include	<sys/sem.h>

#define	KEY	((key_t) 98765L)

#define	PERMS	0666

main()
{
	int	i, semid;

	for (i = 0 ; i < 1000; i++) {
		if ( (semid = semget(KEY, 1, PERMS | IPC_CREAT)) < 0)
			err_sys("can't create semaphore");

		printf("semid = %d\n", semid);

		if (semctl(semid, 0, IPC_RMID, (struct semid_ds *) 0) < 0)
			err_sys("can't remove semaphore");
	}
}
