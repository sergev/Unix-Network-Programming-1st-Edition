/*
 * Copy the existing mode of a terminal to another terminal.
 * Typically this is used to initialize a slave pseudo-terminal
 * to the state of the terminal associated with standard input.
 *
 * We provide two functions to do this in 2 separate steps.
 */

#include	<sys/types.h>
#include	<sys/ioctl.h>

	/* See the tty(4) man page for all the details */
static struct sgttyb	tty_sgttyb;	/* basic modes (V6 & V7) */
static struct tchars	tty_tchars;	/* basic control chars (V7) */
static struct ltchars	tty_ltchars;	/* control chars for new discipline */
static struct winsize	tty_winsize;	/* terminal and window sizes */
static int		tty_localmode;	/* local mode word */
static int		tty_ldisc;	/* line discipline word */

/*
 * Get a copy of the tty modes for a given file descriptor.
 * The copy is then used later by tty_setmode() below.
 */

int
tty_getmode(oldfd)
int	oldfd;		/* typically an actual terminal device */
{
	if (ioctl(oldfd, TIOCGETP,   (char *) &tty_sgttyb) < 0)     return(-1);
	if (ioctl(oldfd, TIOCGETC,   (char *) &tty_tchars) < 0)     return(-1);
	if (ioctl(oldfd, TIOCGLTC,   (char *) &tty_ltchars) < 0)    return(-1);
	if (ioctl(oldfd, TIOCLGET,   (char *) &tty_localmode) < 0)  return(-1);
	if (ioctl(oldfd, TIOCGETD,   (char *) &tty_ldisc) < 0)      return(-1);
	if (ioctl(oldfd, TIOCGWINSZ, (char *) &tty_winsize) < 0)    return(-1);

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
	if (ioctl(newfd, TIOCSETP,   (char *) &tty_sgttyb) < 0)     return(-1);
	if (ioctl(newfd, TIOCSETC,   (char *) &tty_tchars) < 0)     return(-1);
	if (ioctl(newfd, TIOCSLTC,   (char *) &tty_ltchars) < 0)    return(-1);
	if (ioctl(newfd, TIOCLSET,   (char *) &tty_localmode) < 0)  return(-1);
	if (ioctl(newfd, TIOCSETD,   (char *) &tty_ldisc) < 0)      return(-1);
	if (ioctl(newfd, TIOCSWINSZ, (char *) &tty_winsize) < 0)    return(-1);

	return(0);
}
