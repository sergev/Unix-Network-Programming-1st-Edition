/*
 * Pseudo-terminal routines for Unix System V Release 3.2.
 */

#include	<stdio.h>
#include	<fcntl.h>
#include	<stropts.h>

#define	PTY_MASTER	"/dev/ptmx"	/* System V Release 3.2 */

int			/* returns the file descriptor, or -1 on error */
pty_master()
{
	int	master_fd;

	/*
	 * Open the master half - "/dev/ptms".  This is a streams clone
	 * device, so it'll allocate the first available pty master.
	 */

	if ( (master_fd = open(PTY_MASTER, O_RDWR)) < 0)
		return(-1);

	return(master_fd);
}

/*
 * Open the slave half of a pseudo-terminal.
 */

int			/* returns the file descriptor, or -1 on error */
pty_slave(master_fd)
int	master_fd;		/* from pty_master() */
{
	int	slave_fd;
	char	*slavename;
	int	grantpt();	/* undocumented function - libpt.a */
	int	unlockpt();	/* undocumented function - libpt.a */
	char	*ptsname();	/* undocumented function - libpt.a */

	if (grantpt(master_fd) < 0) {	/* change permissions of slave */
		close(master_fd);
		return(-1);
	}

	if (unlockpt(master_fd) < 0) {	/* unlock slave */
		close(master_fd);
		return(-1);
	}

	slavename = ptsname(master_fd);	/* determine the slave's name */
	if (slavename == NULL) {
		close(master_fd);
		return(-1);
	}

	slave_fd = open(slavename, O_RDWR);	/* open the slave */
	if (slave_fd < 0) {
		close(master_fd);
		return(-1);
	}

	/*
	 * Now push two modules onto the slave's stream: "ptem" is the
	 * pseudo-terminal hardware emulation module, and "ldterm" is
	 * the standard terminal line discipline.
	 */

	if (ioctl(slave_fd, I_PUSH, "ptem") < 0) {
		close(master_fd);
		return(-1);
	}
	if (ioctl(slave_fd, I_PUSH, "ldterm") < 0) {
		close(master_fd);
		return(-1);
	}

	return(slave_fd);
}
