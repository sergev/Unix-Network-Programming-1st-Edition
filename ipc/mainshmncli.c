#include	<stdio.h>
#include	<sys/types.h>
#include	<sys/ipc.h>
#include	<sys/shm.h>

#include	"shm.h"

int	clisem, servsem;	/* semaphore IDs */

int	shmid[NBUFF];		/* shared memory IDs */
Mesg	*mesgptr[NBUFF];	/* ptr to message structures, which are
				   in the shared memory segment */

main()
{
	register int	i;

	/*
	 * Get the shared memory segments and attach them.
	 * We don't specify IPC_CREAT, assuming the server creates them.
	 */

	for (i = 0; i < NBUFF; i++) {
		if ( (shmid[i] = shmget(SHMKEY + i, sizeof(Mesg), 0)) < 0)
			err_sys("client: can't get shared memory %d", i);
		if ( (mesgptr[i] = (Mesg *) shmat(shmid[i], (char *) 0, 0))
								== (Mesg *) -1)
			err_sys("client: can't attach shared memory %d", i);
	}

	/*
	 * Open the two semaphores.
	 */

	if ( (clisem = sem_open(SEMKEY1)) < 0)
		err_sys("client: can't open client semaphore");
	if ( (servsem = sem_open(SEMKEY2)) < 0)
		err_sys("client: can't open server semaphore");

	client();

	/*
	 * Detach and remove the shared memory segments and
	 * close the semaphores.
	 */

	for (i = 0; i < NBUFF; i++) {
		if (shmdt(mesgptr[i]) < 0)
			err_sys("client: can't detach shared memory %d", i);
		if (shmctl(shmid[i], IPC_RMID, (struct shmid_ds *) 0) < 0)
			err_sys("client: can't remove shared memory %d", i);
	}

	sem_close(clisem);
	sem_close(servsem);

	exit(0);
}

client()
{
	int	i, n;

	/*
	 * Read the filename from standard input, write it to shared memory.
	 */

	sem_wait(clisem);		/* wait for server to initialize */
	if (fgets(mesgptr[0]->mesg_data, MAXMESGDATA, stdin) == NULL)
		err_sys("filename read error");

	n = strlen(mesgptr[0]->mesg_data);
	if (mesgptr[0]->mesg_data[n-1] == '\n')
		n--;			/* ignore newline from fgets() */
	mesgptr[0]->mesg_len = n;
	sem_signal(servsem);		/* wake up server */

	for ( ; ; ) {
		for (i = 0; i < NBUFF; i++) {
			sem_wait(clisem);
			if ( (n = mesgptr[i]->mesg_len) <= 0)
				goto alldone;
			if (write(1, mesgptr[i]->mesg_data, n) != n)
				err_sys("data write error");
			sem_signal(servsem);
		}
	}

alldone:
	if (n < 0)
		err_sys("data read error");
}
