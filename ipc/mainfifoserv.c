#include	"fifo.h"

main()
{
	int	readfd, writefd;

	/*
	 * Create the FIFOs, then open them - one for reading and one
	 * for writing.
	 */

	if ( (mknod(FIFO1, S_IFIFO | PERMS, 0) < 0) && (errno != EEXIST))
		err_sys("can't create fifo: %s", FIFO1);
	if ( (mknod(FIFO2, S_IFIFO | PERMS, 0) < 0) && (errno != EEXIST)) {
		unlink(FIFO1);
		err_sys("can't create fifo: %s", FIFO2);
	}

	if ( (readfd = open(FIFO1, 0)) < 0)
		err_sys("server: can't open read fifo: %s", FIFO1);
	if ( (writefd = open(FIFO2, 1)) < 0)
		err_sys("server: can't open write fifo: %s", FIFO2);

	server(readfd, writefd);

	close(readfd);
	close(writefd);

	exit(0);
}
