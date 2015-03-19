/*
 * Create a named stream pipe.
 */

#include	<sys/types.h>
#include	<sys/stat.h>

int					/* return 0 if OK, -1 on error */
ns_pipe(name, fd)
char	*name;		/* user-specified name to assign to the stream pipe */
int	fd[2];		/* two file descriptors returned through here */
{
	int		omask;
	struct stat	statbuff;

	/*
	 * First create an unnamed stream pipe.
	 */

	if (s_pipe(fd) < 0)
		return(-1);

	/*
	 * Now assign the name to one end (the first descriptor, fd[0]).
	 * To do this we first find its major/minor device numbers using
	 * fstat(2), then use these in a call to mknod(2) to create the
	 * filesystem entry.  Beware that mknod(2) is restricted to root
	 * (for everything other than FIFOs).
	 * Under System VR3.2, the major value for the unnamed stream pipe
	 * corresponds to the major for "/dev/spx" and the minor value is
	 * whatever the "/dev/spx" clone driver assigned to make the
	 * major/minor unique.
	 */

	if (fstat(fd[0], &statbuff) < 0) {
		close(fd[0]);
		close(fd[1]);
		return(-1);
	}

	unlink(name);			/* in case it already exists */
	omask = umask(0);		/* assure mode is 0666 */
	if (mknod(name, S_IFCHR | 0666, statbuff.st_rdev) < 0) {
		close(fd[0]);
		close(fd[1]);
		umask(omask);
		return(-1);
	}
	umask(omask);			/* restore old umask value */

	return(0);		/* all OK */
}
