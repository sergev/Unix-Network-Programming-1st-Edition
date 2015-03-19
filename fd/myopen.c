/*
 * Open a file, returning a file descriptor.
 *
 * This function is similar to the UNIX open() system call,
 * however here we invoke another program to do the actual
 * open(), to illustrate the passing of open files
 * between processes.
 */

int
my_open(filename, mode)
char	*filename;
int	mode;
{
	int		fd, childpid, sfd[2], status;
	char		argsfd[10], argmode[10];
	extern int	errno;

	if (s_pipe(sfd) < 0)		/* create an unnamed stream pipe */
		return(-1);		/* errno will be set */

	if ( (childpid = fork()) < 0)
		err_sys("can't fork");

	else if (childpid == 0) {	/* child process */
		close(sfd[0]);		/* close the end we don't use */
		sprintf(argsfd, "%d", sfd[1]);
		sprintf(argmode, "%d", mode);
		if (execl("./openfile", "openfile", argsfd, filename,
			  argmode, (char *) 0) < 0)
				err_sys("can't execl");
	}

	/* parent process - wait for the child's execl() to complete */

	close(sfd[1]);			/* close the end we don't use */
	if (wait(&status) != childpid)
		err_dump("wait error");
	if ((status & 255) != 0)
		err_dump("child did not exit");
	status = (status >> 8) & 255;		/* child's exit() argument */
	if (status == 0) {
		fd = recvfile(sfd[0]);		/* all OK, receive fd */
	} else {
		errno = status;	/* error, set errno value from child's errno */
		fd = -1;
	}

	close(sfd[0]);		/* close the stream pipe */
	return(fd);
}
