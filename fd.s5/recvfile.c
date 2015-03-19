/*
 * Receive a file descriptor from another process.
 * Return the file descriptor if OK, otherwise return -1.
 */

#include	<sys/types.h>
#include	<stropts.h>

int
recvfile(sfd)
int	sfd;		/* stream pipe */
{
	int			fd;
	struct strrecvfd	recv;

	if (ioctl(sfd, I_RECVFD, (char *) &recv) < 0)
		return(-1);

	return(recv.fd);	/* return the new file descriptor */
		/* we don't return the uid and gid that are available */
}
