/*
 * Create an unnamed stream pipe.
 */

#include	<sys/types.h>
#include	<sys/socket.h>

int			/* returns 0 if all OK, -1 if error (with errno set) */
s_pipe(fd)
int	fd[2];		/* two file descriptors returned through here */
{
	return( socketpair(AF_UNIX, SOCK_STREAM, 0, fd) );
}
