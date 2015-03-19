#include	<sys/types.h>
#include	<sys/stat.h>
#include	<sys/errno.h>
extern int	errno;

#define	FIFO1	"/tmp/fifo.1"
#define	FIFO2	"/tmp/fifo.2"
#define	PERMS	0666

main()
{
	int	childpid, readfd, writefd;

	if ( (mknod(FIFO1, S_IFIFO | PERMS, 0) < 0) && (errno != EEXIST))
		err_sys("can't create fifo 1: %s", FIFO1);
	if ( (mknod(FIFO2, S_IFIFO | PERMS, 0) < 0) && (errno != EEXIST)) {
		unlink(FIFO1);
		err_sys("can't create fifo 2: %s", FIFO2);
	}

	if ( (childpid = fork()) < 0) {
		err_sys("can't fork");

	} else if (childpid > 0) {			/* parent */
		if ( (writefd = open(FIFO1, 1)) < 0)
			err_sys("parent: can't open write fifo");
		if ( (readfd = open(FIFO2, 0)) < 0)
			err_sys("parent: can't open read fifo");

		client(readfd, writefd);

		while (wait((int *) 0) != childpid)	/* wait for child */
			;

		close(readfd);
		close(writefd);

		if (unlink(FIFO1) < 0)
			err_sys("parent: can't unlink %s", FIFO1);
		if (unlink(FIFO2) < 0)
			err_sys("parent: can't unlink %s", FIFO2);
		exit(0);

	} else {				/* child */
		if ( (readfd = open(FIFO1, 0)) < 0)
			err_sys("child: can't open read fifo");
		if ( (writefd = open(FIFO2, 1)) < 0)
			err_sys("child: can't open write fifo");

		server(readfd, writefd);

		close(readfd);
		close(writefd);
		exit(0);
	}
}
