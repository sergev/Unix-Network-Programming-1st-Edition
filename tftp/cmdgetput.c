/*
 * File get/put processing.
 *
 * This is the way the client side gets started - either the user
 * wants to get a file (generates a RRQ command to the server)
 * or the user wants to put a file (generates a WRQ command to the
 * server).  Once either the RRQ or the WRQ command is sent,
 * the finite state machine takes over the transmission.
 */

#include	"defs.h"

/*
 * Execute a get command - read a remote file and store on the local system.
 */

do_get(remfname, locfname)
char	*remfname;
char	*locfname;
{
	if ( (localfp = file_open(locfname, "w", 1)) == NULL) {
		err_ret("can't fopen %s for writing", locfname);
		return;
	}

	if (net_open(hostname, TFTP_SERVICE, port) < 0)
		return;

	totnbytes = 0;
	t_start();		/* start timer for statistics */

	send_RQ(OP_RRQ, remfname, modetype);

	fsm_loop(OP_RRQ);

	t_stop();		/* stop timer for statistics */

	net_close();

	file_close(localfp);

	printf("Received %ld bytes in %.1f seconds\n", totnbytes, t_getrtime());
				/* print stastics */
}

/*
 * Execute a put command - send a local file to the remote system.
 */

do_put(remfname, locfname)
char	*remfname;
char	*locfname;
{
	if ( (localfp = file_open(locfname, "r", 0)) == NULL) {
		err_ret("can't fopen %s for reading", locfname);
		return;
	}

	if (net_open(hostname, TFTP_SERVICE, port) < 0)
		return;

	totnbytes = 0;
	t_start();		/* start timer for statistics */

	lastsend = MAXDATA;
	send_RQ(OP_WRQ, remfname, modetype);

	fsm_loop(OP_WRQ);

	t_stop();		/* stop timer for statistics */

	net_close();

	file_close(localfp);

	printf("Sent %ld bytes in %.1f seconds\n", totnbytes, t_getrtime());
				/* print stastics */
}
