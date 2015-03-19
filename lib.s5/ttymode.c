/*
 * Copy the existing mode of a terminal to another terminal.
 * Typically this is used to initialize a slave pseudo-terminal
 * to the state of the terminal associated with standard input.
 *
 * We provide two functions to do this in 2 separate steps.
 */

#include	<termio.h>

	/* See the termio(7) man page for all the details */
static struct termio	tty_termio;	/* System V's structure */

/*
 * Get a copy of the tty modes for a given file descriptor.
 * The copy is then used later by tty_setmode() below.
 */

int
tty_getmode(oldfd)
int	oldfd;		/* typically an actual terminal device */
{
	if (ioctl(oldfd, TCGETA, (char *) &tty_termio) < 0)
		return(-1);

	return(0);
}

/*
 * Set the tty modes for a given file descriptor.
 * We set the modes from the values saved by tty_getmode() above.
 */

int
tty_setmode(newfd)
int	newfd;		/* typically a pseudo-terminal slave device */
{
	if (ioctl(newfd, TCSETA, (char *) &tty_termio) < 0)
		return(-1);

	return(0);
}
