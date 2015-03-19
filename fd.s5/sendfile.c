/*
 * Pass a file descriptor to another process.
 * Return 0 if OK, otherwise return the errno value from the I_SENDFD ioctl.
 */

#include	<sys/types.h>
#include	<stropts.h>

int
sendfile(strfd, fd)
int	strfd;		/* stream pipe to pass descriptor on */
int	fd;		/* the actual fd value to pass */
{
	if (ioctl(strfd, I_SENDFD, fd) < 0)
		return(-1);

	return(0);
}
