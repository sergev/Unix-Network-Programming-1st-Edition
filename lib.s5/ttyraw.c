/*
 * Put a terminal device into RAW mode with ECHO off.
 * Before doing so we first save the terminal's current mode,
 * assuming the caller will call the tty_reset() function
 * (also in this file) when it's done with raw mode.
 */

#include	<termio.h>

static struct termio	tty_mode;	/* save tty mode here */

int
tty_raw(fd)
int	fd;		/* of terminal device */
{
	struct termio	temp_mode;

	if (ioctl(fd, TCGETA, (char *) &temp_mode) < 0)
		return(-1);
	tty_mode = temp_mode;		/* save for restoring later */

	temp_mode.c_iflag  = 0;		/* turn off all input control */
	temp_mode.c_oflag &= ~OPOST;	/* disable output post-processing */
	temp_mode.c_lflag &= ~(ISIG | ICANON | ECHO | XCASE);
					/* disable signal generation */
 					/* disable canonical input */
					/* disable echo */
					/* disable upper/lower output */
	temp_mode.c_cflag &= ~(CSIZE | PARENB);
					/* clear char size, disable parity */
	temp_mode.c_cflag |= CS8;	/* 8-bit chars */

	temp_mode.c_cc[VMIN]  = 1;	/* min #chars to satisfy read */
	temp_mode.c_cc[VTIME] = 1;	/* 10'ths of seconds between chars */

	if (ioctl(fd, TCSETA, (char *) &temp_mode) < 0)
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
	if (ioctl(fd, TCSETA, (char *) &tty_mode) < 0)
		return(-1);

	return(0);
}
