/*
 * This function copies standard input to "fd" and also copies
 * everything from "fd" to standard output.
 * In addition, all this data that is copied could also
 * be recorded in a log file, if desired.
 *
 * This version fork's into two processes - one to copy stdin to fd,
 * and one to copy fd to stdout.  This is required if you can't use
 * select(2) or poll(2) on any of the three descriptors.
 */

#include	<sys/types.h>
#include	<signal.h>

#define	BUFFSIZE	512

static int	sigcaught;	/* set by signal handler */

pass_all(fd, childpid)
int	fd;
int	childpid;
{
	int		newpid, nread;
	int		sig_term();
	char		buff[BUFFSIZE];

	if ( (newpid = fork()) < 0) {
		err_sys("parent1: can't fork");

	} else if (newpid == 0) {	/* child: stdin -> fd */
		for ( ; ; ) {
			nread = read(0, buff, BUFFSIZE);
			if (nread < 0)
				err_sys("read error from stdin");
			else if (nread == 0)
				break;		/* stdin EOF -> done */

			if (writen(fd, buff, nread) != nread)
				err_sys("writen error to stream pipe");
		}
		kill(getppid(), SIGTERM);	/* kill parent */
		exit(0);
	}
	/* parent: fd -> stdout */

#ifdef	SIGTTIN
	siginterrupt(SIGTERM, 1);	/* interrupt the system call */
#endif

	sigcaught = 0;
	signal(SIGTERM, sig_term);

	for ( ; ; ) {
		if ( (nread = read(fd, buff, BUFFSIZE)) <= 0)
			break;		/* error, EOF or signal; terminate */

		if (write(1, buff, nread) != nread)
			err_sys("write error to stdout");
	}

	/*
	 * If we get here either there was an EOF on the stream pipe,
	 * implying that the shell terminated, or the child process above
	 * terminated and we received its SIGTERM.
	 * If the shell terminated, we have to let the child process
	 * that we fork'ed above know this, so that it can break out
	 * of its read from the standard input.
	 * If we received the signal, the child is already gone, so we're done.
	 */

	if (sigcaught == 0)
		kill(newpid, SIGTERM);

	return;		/* parent returns to caller */
}
/*
 * If we get here, the child that was copying stdin to the IPC
 * channel got an EOF or error.  It has notified us with a SIGTERM
 * signal.  We set a flag for the parent process above.
 */

sig_term()
{
	sigcaught = 1;		/* set flag */
}
