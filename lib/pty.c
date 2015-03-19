/*
 * Pseudo-terminal routines for 4.3BSD.
 */

#include	<sys/types.h>
#include	<sys/stat.h>
#include	<sys/file.h>

/*
 * The name of the pty master device is stored here by pty_master().
 * The next call to pty_slave() uses this name for the slave.
 */

static char	pty_name[12];	/* "/dev/[pt]tyXY" = 10 chars + null byte */

int			/* returns the file descriptor, or -1 on error */
pty_master()
{
	int		i, master_fd;
	char		*ptr;
	struct stat	statbuff;
	static char	ptychar[] = "pqrs";			/* X */
	static char	hexdigit[] = "0123456789abcdef";	/* Y */

	/*
	 * Open the master half - "/dev/pty[pqrs][0-9a-f]".
	 * There is no easy way to obtain an available minor device
	 * (similar to a streams clone open) - we have to try them
	 * all until we find an unused one.
	 */

	for (ptr = ptychar; *ptr != 0; ptr++) {
		strcpy(pty_name, "/dev/ptyXY");
		pty_name[8] = *ptr;	/* X */
		pty_name[9] = '0';	/* Y */

		/*
		 * If this name, "/dev/ptyX0" doesn't even exist,
		 * then we can quit now.  It means the system doesn't
		 * have /dev entries for this group of 16 ptys.
		 */

		if (stat(pty_name, &statbuff) < 0)
			break;

		for (i = 0; i < 16; i++) {
			pty_name[9] = hexdigit[i];	/* 0-15 -> 0-9a-f */
			if ( (master_fd = open(pty_name, O_RDWR)) >= 0)
				return(master_fd);	/* got it, done */
		}
	}
	return(-1);	/* couldn't open master, assume all pty's are in use */
}

/*
 * Open the slave half of a pseudo-terminal.
 * Note that the master half of a pty is a single-open device,
 * so there isn't a race condition between opening the master
 * above and opening the slave below.  The only way the slave
 * open will fail is if someone has opened the slave without
 * first opening the master.
 */

int			/* returns the file descriptor, or -1 on error */
pty_slave(master_fd)
int	master_fd;		/* from pty_master() */
{
	int	slave_fd;

	pty_name[5] = 't';	/* change "/dev/ptyXY" to "/dev/ttyXY" */
	if ( (slave_fd = open(pty_name, O_RDWR)) < 0) {
		close(master_fd);
		return(-1);
	}

	return(slave_fd);
}
