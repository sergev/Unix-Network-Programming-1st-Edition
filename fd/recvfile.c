/*
 * Receive a file descriptor from another process.
 * Return the file descriptor if OK, otherwise return -1.
 */

#include	<sys/types.h>
#include	<sys/socket.h>
#include	<sys/uio.h>

int
recvfile(sockfd)
int	sockfd;		/* UNIX domain socket to receive descriptor on */
{
	int		fd;
	struct iovec	iov[1];
	struct msghdr	msg;

	iov[0].iov_base = (char *) 0;		/* no data to receive */
	iov[0].iov_len  = 0;
	msg.msg_iov          = iov;
	msg.msg_iovlen       = 1;
	msg.msg_name         = (caddr_t) 0;
	msg.msg_accrights    = (caddr_t) &fd;	/* address of descriptor */
	msg.msg_accrightslen = sizeof(fd);	/* receive 1 descriptor */

	if (recvmsg(sockfd, &msg, 0) < 0)
		return(-1);

	return(fd);
}
