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
	 * Create the shared memory segment, if required,
	 * then attach it.
	 */

	if ( (shmid = shmget(SHMKEY, sizeof(Mesg), PERMS | IPC_CREAT)) < 0)
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
	int	n, filefd;
	char	errmesg[256], *sys_err_str();

	/*
	 * Wait for the client to write the filename into shared memory.
	 */

	sem_wait(servsem);	/* we'll wait here for client to start things */

	mesgptr->mesg_data[mesgptr->mesg_len] = '\0';
					/* null terminate filename */

	if ( (filefd = open(mesgptr->mesg_data, 0)) < 0) {
		/*
		 * Error.  Format an error message and send it back
		 * to the client.
		 */

		sprintf(errmesg, ": can't open, %s\n", sys_err_str());
		strcat(mesgptr->mesg_data, errmesg);
		mesgptr->mesg_len = strlen(mesgptr->mesg_data);
		sem_signal(clisem);		/* send to client */
		sem_wait(servsem);		/* wait for client to process */

	} else {
		/*
		 * Read the data from the file right into shared memory.
		 * The -1 in the number-of-bytes-to-read is because some
		 * Unices have a bug if you try and read into the final byte
		 * of a shared memory segment.
		 */

		while ( (n = read(filefd, mesgptr->mesg_data,
							MAXMESGDATA-1)) > 0) {
			mesgptr->mesg_len = n;
			sem_signal(clisem);	/* send to client */
			sem_wait(servsem);	/* wait for client to process */
		}
		close(filefd);
		if (n < 0)
			err_sys("server: read error");
	}

	/*
	 * Send a message with a length of 0 to signify the end.
	 */

	mesgptr->mesg_len = 0;
	sem_signal(clisem);
}
