#include	"mesg.h"

/*
 * Send a message by writing on a file descriptor.
 * The mesg_len, mesg_type and mesg_data fields must be filled
 * in by the caller.
 */

mesg_send(fd, mesgptr)
int	fd;
Mesg	*mesgptr;
{
	int	n;

	/*
	 * Write the message header and the optional data.
	 * First calculate the length of the length field, plus the
	 * type field, plus the optional data.
	 */

	n = MESGHDRSIZE + mesgptr->mesg_len;

	if (write(fd, (char *) mesgptr, n) != n)
		err_sys("message write error");
}

/*
 * Receive a message by reading on a file descriptor.
 * Fill in the mesg_len, mesg_type and mesg_data fields, and return
 * mesg_len as the return value also.
 */

int
mesg_recv(fd, mesgptr)
int	fd;
Mesg	*mesgptr;
{
	int	n;

	/*
	 * Read the message header first.  This tells us how much
	 * data follows the message for the next read.
	 * An end-of-file on the file descriptor causes us to
	 * return 0.  Hence, we force the assumption on the caller
	 * that a 0-length message means EOF.
	 */

	if ( (n = read(fd, (char *) mesgptr, MESGHDRSIZE)) == 0)
		return(0);		/* end of file */
	else if (n != MESGHDRSIZE)
		err_sys("message header read error");

	/*
	 * Read the actual data, if there is any.
	 */

	if ( (n = mesgptr->mesg_len) > 0)
		if (read(fd, mesgptr->mesg_data, n) != n)
			err_sys("data read error");
	return(n);
}
