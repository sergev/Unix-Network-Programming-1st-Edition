#include	<stdio.h>
#include	<sys/types.h>
#include	<sys/ipc.h>
#include	<sys/shm.h>

#include	"shm.h"

#define	NUMMESG	10000
#define	MESGLEN	2048

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
	int	i;

	/*
	 * Tell the server we're here and ready,
	 * then just receive while the server sends us things.
	 */

	sem_wait(clisem);		/* get control of shared memory */
	mesgptr->mesg_len = MESGLEN;
	mesgptr->mesg_type = MESGLEN;
	sem_signal(servsem);		/* wake up server */

	for (i = 0; i < NUMMESG; i++) {
		sem_wait(clisem);	/* wait for server */

		if (mesgptr->mesg_len != MESGLEN)
			err_sys("client: incorrect length");
		if (mesgptr->mesg_type != (i + 1))
			err_sys("client: incorrect type");

		sem_signal(servsem);	/* wake up server */
	}
}
