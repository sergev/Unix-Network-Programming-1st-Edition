#include	<stdio.h>
#include	<sys/types.h>
#include	<sys/ipc.h>
#include	<sys/shm.h>

#include	"shm.h"

int	shmid, clisem, servsem;	/* shared memory and semaphore IDs */
Mesg	*mesgptr;		/* ptr to message structure, which is
				   in the shared memory segment */

main()
{
	/*
	 * Get the shared memory segment and attach it.
	 * The server must have already created it.
	 */

	if ( (shmid = shmget(SHMKEY, sizeof(Mesg), 0)) < 0)
		err_sys("client: can't get shared memory segment");
	if ( (mesgptr = (Mesg *) shmat(shmid, (char *) 0, 0)) == (Mesg *) -1)
		err_sys("client: can't attach shared memory segment");

	/*
	 * Open the two semaphores.  The server must have
	 * created them already.
	 */

	if ( (clisem = sem_open(SEMKEY1)) < 0)
		err_sys("client: can't open client semaphore");
	if ( (servsem = sem_open(SEMKEY2)) < 0)
		err_sys("client: can't open server semaphore");

	client();

	/*
	 * Detach and remove the shared memory segment and
	 * close the semaphores.
	 */

	if (shmdt(mesgptr) < 0)
		err_sys("client: can't detach shared memory");
	if (shmctl(shmid, IPC_RMID, (struct shmid_ds *) 0) < 0)
		err_sys("client: can't remove shared memory");

	sem_close(clisem);	/* will remove the semaphore */
	sem_close(servsem);	/* will remove the semaphore */

	exit(0);
}

client()
{
	int	n;

	/*
	 * Read the filename from standard input, write it to shared memory.
	 */

	sem_wait(clisem);		/* get control of shared memory */
	if (fgets(mesgptr->mesg_data, MAXMESGDATA, stdin) == NULL)
		err_sys("filename read error");

	n = strlen(mesgptr->mesg_data);
	if (mesgptr->mesg_data[n-1] == '\n')
		n--;			/* ignore newline from fgets() */
	mesgptr->mesg_len = n;
	sem_signal(servsem);		/* wake up server */

	/*
	 * Wait for the server to place something in shared memory.
	 */

	sem_wait(clisem);		/* wait for server to process */
	while( (n = mesgptr->mesg_len) > 0) {
		if (write(1, mesgptr->mesg_data, n) != n)
			err_sys("data write error");
		sem_signal(servsem);	/* wake up server */
		sem_wait(clisem);	/* wait for server to process */
	}

	if (n < 0)
		err_sys("data read error");
}
