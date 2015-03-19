#include	"fifo.h"

main()
{
	int	readfd, writefd;

	/*
	 * Open the FIFOs.  We assume the server has already created them.
	 */

	if ( (writefd = open(FIFO1, 1)) < 0)
		err_sys("client: can't open write fifo: %s", FIFO1);
	if ( (readfd = open(FIFO2, 0)) < 0)
		err_sys("client: can't open read fifo: %s", FIFO2);

	client(readfd, writefd);

	close(readfd);
	close(writefd);

	/*
	 * Delete the FIFOs, now that we're finished.
	 */

	if (unlink(FIFO1) < 0)
		err_sys("client: can't unlink %s", FIFO1);
	if (unlink(FIFO2) < 0)
		err_sys("client: can't unlink %s", FIFO2);

	exit(0);
}
