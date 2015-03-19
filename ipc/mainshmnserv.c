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
	 */

	for (i = 0; i < NBUFF; i++) {
		if ( (shmid[i] = shmget(SHMKEY + i, sizeof(Mesg),
						PERMS | IPC_CREAT)) < 0)
			err_sys("server: can't get shared memory %d", i);
		if ( (mesgptr[i] = (Mesg *) shmat(shmid[i], (char *) 0, 0))
							== (Mesg *) -1)
			err_sys("server: can't attach shared memory %d", i);
	}

	/*
	 * Create the two semaphores.
	 */

	if ( (clisem = sem_create(SEMKEY1, 1)) < 0)
		err_sys("server: can't create client semaphore");
	if ( (servsem = sem_create(SEMKEY2, 0)) < 0)
		err_sys("server: can't create server semaphore");

	server();

	/*
	 * Detach the shared memory segments and close the semaphores.
	 * We let the client remove the shared memory segments,
	 * since it'll be the last one to use them.
	 */

	for (i = 0; i < NBUFF; i++) {
		if (shmdt(mesgptr[i]) < 0)
			err_sys("server: can't detach shared memory %d", i);
	}

	sem_close(clisem);
	sem_close(servsem);

	exit(0);
}

server()
{
	register int	i, n, filefd;
	char		errmesg[256], *sys_err_str();

	/*
	 * Wait for the client to write the filename into shared memory,
	 * then try to open the file.
	 */

	sem_wait(servsem);
	mesgptr[0]->mesg_data[mesgptr[0]->mesg_len] = '\0';
					/* null terminate filename */
	if ( (filefd = open(mesgptr[0]->mesg_data, 0)) < 0) {
		/*
		 * Error.  Format an error message and send it back
		 * to the client.
		 */

		sprintf(errmesg, ": can't open, %s\n", sys_err_str());
		strcat(mesgptr[0]->mesg_data, errmesg);
		mesgptr[0]->mesg_len = strlen(mesgptr[0]->mesg_data);
		sem_signal(clisem);	/* wake up client */

		sem_wait(servsem);	/* wait for client to process */
		mesgptr[1]->mesg_len = 0;
		sem_signal(clisem);	/* wake up client */

	} else {
		/*
		 * Initialize the server semaphore to the number
		 * of buffers.  We know its value is 0 now, since
		 * it was initialized to 0, and the client has done a
		 * sem_signal(), followed by our sem_wait() above.
		 * What we do is increment the semaphore value
		 * once for every buffer (i.e., the number of resources
		 * that we have).
		 */

		for (i = 0; i < NBUFF; i++)
			sem_signal(servsem);

		/*
		 * Read the data from the file right into shared memory.
		 * The -1 in the number-of-bytes-to-read is because some
		 * Unices have a bug if you try and read into the final byte
		 * of a shared memory segment.
		 */

		for ( ; ; ) {
			for (i = 0; i < NBUFF; i++) {
				sem_wait(servsem);
				n = read(filefd, mesgptr[i]->mesg_data,
							MAXMESGDATA-1);
				if (n < 0)
					err_sys("server: read error");
				mesgptr[i]->mesg_len = n;
				sem_signal(clisem);
				if (n == 0)
					goto alldone;
			}
		}
alldone:
		/* we've already written the 0-length final buffer */
		close(filefd);
	}
}
