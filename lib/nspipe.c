/*
 * Create a named stream pipe.
 */

#include	<sys/types.h>
#include	<sys/socket.h>
#include	<sys/un.h>

int			/* returns 0 if all OK, -1 if error (with errno set) */
ns_pipe(name, fd)
char	*name;		/* user-specified name to assign to the stream pipe */
int	fd[2];		/* two file descriptors returned through here */
{
	int			len;
	struct sockaddr_un	unix_addr;

	if (s_pipe(fd) < 0)	/* first create an unnamed stream pipe */
		return(-1);

	unlink(name);	/* remove the name, if it already exists */

	bzero((char *) &unix_addr, sizeof(unix_addr));
	unix_addr.sun_family = AF_UNIX;
	strcpy(unix_addr.sun_path, name);
	len = strlen(unix_addr.sun_path) + sizeof(unix_addr.sun_family);

	if (bind(fd[0], (struct sockaddr *) &unix_addr, len) < 0)
		return(-1);

	return(0);
}
