/*
 * Copy standard input to standard output, using asynchronous I/O.
 */

#include	<signal.h>
#include	<fcntl.h>

#define	BUFFSIZE	4096

int	sigflag;

main()
{
	int		n;
	char		buff[BUFFSIZE];
	int		sigio_func();

	signal(SIGIO, sigio_func);

	if (fcntl(0, F_SETOWN, getpid()) < 0)
		err_sys("F_SETOWN error");

	if (fcntl(0, F_SETFL, FASYNC) < 0)
		err_sys("F_SETFL FASYNC error");

	for ( ; ; ) {
		sigblock(sigmask(SIGIO));
		while (sigflag == 0)
			sigpause(0);		/* wait for a signal */

		/*
		 * We're here if (sigflag != 0).  Also, we know that the
		 * SIGIO signal is currently blocked.
		 */

		if ( (n = read(0, buff, BUFFSIZE)) > 0) {
			if (write(1, buff, n) != n)
				err_sys("write error");
		} else if (n < 0)
			err_sys("read error");
		else if (n == 0)
			exit(0);		/* EOF */

		sigflag = 0;			/* turn off our flag */

		sigsetmask(0);			/* and reenable signals */
	}
}

int
sigio_func()
{
	sigflag = 1;		/* just set flag and return */

	/* the 4.3BSD signal facilities leave this handler enabled
	   for any further SIGIO signals. */
}
