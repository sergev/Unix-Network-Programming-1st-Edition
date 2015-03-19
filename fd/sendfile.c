/*
 * Pass a file descriptor to another process.
 * Return 0 if OK, otherwise return the errno value from the
 * sendmsg() system call.
 */

#include	<sys/types.h>
#include	<sys/socket.h>
#include	<sys/uio.h>

int
sendfile(sockfd, fd)
int	sockfd;		/* UNIX domain socket to pass descriptor on */
int	fd;		/* the actual fd value to pass */
{
	struct iovec	iov[1];
	struct msghdr	msg;
	extern int	errno;

	iov[0].iov_base = (char *) 0;		/* no data to send */
	iov[0].iov_len  = 0;
	msg.msg_iov          = iov;
	msg.msg_iovlen       = 1;
	msg.msg_name         = (caddr_t) 0;
	msg.msg_accrights    = (caddr_t) &fd;	/* address of descriptor */
	msg.msg_accrightslen = sizeof(fd);	/* pass 1 descriptor */

	if (sendmsg(sockfd, &msg, 0) < 0)
		return( (errno > 0) ? errno : 255 );

	return(0);
}
