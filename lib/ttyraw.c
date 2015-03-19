/*
 * Put a terminal device into RAW mode with ECHO off.
 * Before doing so we first save the terminal's current mode,
 * assuming the caller will call the tty_reset() function
 * (also in this file) when it's done with raw mode.
 */

#include	<sys/types.h>
#include	<sys/ioctl.h>

static struct sgttyb	tty_mode;	/* save tty mode here */

int
tty_raw(fd)
int	fd;		/* of terminal device */
{
	struct sgttyb	temp_mode;

	if (ioctl(fd, TIOCGETP, (char *) &temp_mode) < 0)
		return(-1);
	tty_mode = temp_mode;		/* save for restoring later */

	temp_mode.sg_flags |= RAW;	/* turn RAW mode on */
	temp_mode.sg_flags &= ~ECHO;	/* turn ECHO off */
	if (ioctl(fd, TIOCSETP, (char *) &temp_mode) < 0)
		return(-1);

	return(0);
}

/*
 * Restore a terminal's mode to whatever it was on the most
 * recent call to the tty_raw() function above.
 */

int
tty_reset(fd)
int	fd;		/* of terminal device */
{
	if (ioctl(fd, TIOCSETP, (char *) &tty_mode) < 0)
		return(-1);

	return(0);
}
