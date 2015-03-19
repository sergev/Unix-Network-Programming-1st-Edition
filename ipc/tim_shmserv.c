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
	 * Create the shared memory segment, if required,
	 * then attach it.
	 */

	if ( (shmid = shmget(SHMKEY, sizeof(Mesg), 0666 | IPC_CREAT)) < 0)
		err_sys("server: can't get shared memory");
	if ( (mesgptr = (Mesg *) shmat(shmid, (char *) 0, 0)) == (Mesg *) -1)
		err_sys("server: can't attach shared memory");

	/*
	 * Create two semaphores.  The client semaphore starts out at 1
	 * since the client process starts things going.
	 */

	if ( (clisem = sem_create(SEMKEY1, 1)) < 0)
		err_sys("server: can't create client semaphore");
	if ( (servsem = sem_create(SEMKEY2, 0)) < 0)
		err_sys("server: can't create server semaphore");

	server();

	/*
	 * Detach the shared memory segment and close the semaphores.
	 * The client is the last one to use the shared memory, so
	 * it'll remove it when it's done.
	 */

	if (shmdt(mesgptr) < 0)
		err_sys("server: can't detach shared memory");

	sem_close(clisem);
	sem_close(servsem);

	exit(0);
}

server()
{
	int	i;

	/*
	 * Wait for the client to tell us it's there and ready.
	 */

	sem_wait(servsem);
	if (mesgptr->mesg_len != MESGLEN)
		err_sys("server: incorrect length");
	if (mesgptr->mesg_type != MESGLEN)
		err_sys("server: incorrect type");

	/*
	 * Then send messages to the client.
	 */

	for (i = 0; i < NUMMESG; i++) {
		mesgptr->mesg_len = MESGLEN;
		mesgptr->mesg_type = (i + 1);

		sem_signal(clisem);	/* send to client */
		sem_wait(servsem);	/* wait for client to process */
	}
}
