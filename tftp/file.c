/*
 * Routines to open/close/read/write the local file.
 * For "binary" (octet) transmissions, we use the UNIX open/read/write
 * system calls (or their equivalent).
 * For "ascii" (netascii) transmissions, we use the UNIX standard i/o routines
 * fopen/getc/putc (or their equivalent).
 */

#include	"defs.h"

/*
 * The following are used by the functions in this file only.
 */

static int	lastcr   = 0;	/* 1 if last character was a carriage-return */
static int	nextchar = 0;

/*
 * Open the local file for reading or writing.
 * Return a FILE pointer, or NULL on error.
 */

FILE *
file_open(fname, mode, initblknum)
char	*fname;
char	*mode;		/* for fopen() - "r" or "w" */
int	initblknum;
{
	register FILE	*fp;

	if (strcmp(fname, "-") == 0)
		fp = stdout;
	else if ( (fp = fopen(fname, mode)) == NULL)
		return((FILE *) 0);

	nextblknum = initblknum;	/* for first data packet or first ACK */
	lastcr     = 0;			/* for file_write() */
	nextchar   = -1;		/* for file_read() */

	DEBUG2("file_open: opened %s, mode = %s", fname, mode);

	return(fp);
}

/*
 * Close the local file.
 * This causes the standard i/o system to flush its buffers for this file.
 */

file_close(fp)
FILE	*fp;
{
	if (lastcr)
		err_dump("final character was a CR");
	if (nextchar >= 0)
		err_dump("nextchar >= 0");

	if (fp == stdout)
		return;		/* don't close standard output */
	else if (fclose(fp) == EOF)
		err_dump("fclose error");
}

/*
 * Read data from the local file.
 * Here is where we handle any conversion between the file's mode
 * on the local system and the network mode.
 *
 * Return the number of bytes read (between 1 and maxnbytes, inclusive)
 * or 0 on EOF.
 */

int
file_read(fp, ptr, maxnbytes, mode)
FILE		*fp;
register char	*ptr;
register int	maxnbytes;
int		mode;
{
	register int	c, count;

	if (mode == MODE_BINARY) {
		count = read(fileno(fp), ptr, maxnbytes);
		if (count < 0)
			err_dump("read error on local file");

		return(count);		/* will be 0 on EOF */

	} else if (mode == MODE_ASCII) {
		/*
		 * For files that are transferred in netascii, we must
		 * perform the reverse conversions that file_write() does.
		 * Note that we have to use the global "nextchar" to
		 * remember if the next character to output is a linefeed
		 * or a null, since the second byte of a 2-byte sequence
		 * may not fit in the current buffer, and may have to go
		 * as the first byte of the next buffer (i.e., we have to
		 * remember this fact from one call to the next).
		 */

		for (count = 0; count < maxnbytes; count++) {
			if (nextchar >= 0) {
				*ptr++ = nextchar;
				nextchar = -1;
				continue;
			}

			c = getc(fp);

			if (c == EOF) {	/* EOF return means eof or error */
				if (ferror(fp))
				   err_dump("read err from getc on local file");
				return(count);

			} else if (c == '\n') {
				c = '\r';		/* newline -> CR,LF */
				nextchar = '\n';

			} else if (c == '\r') {
				nextchar = '\0';	/* CR -> CR,NULL */

			} else
				nextchar = -1;

			*ptr++ = c;
		}

		return(count);
	} else
		err_dump("unknown MODE value");

	/* NOTREACHED */
}

/*
 * Write data to the local file.
 * Here is where we handle any conversion between the mode of the
 * file on the network and the local system's conventions.
 */

file_write(fp, ptr, nbytes, mode)
FILE		*fp;
register char	*ptr;
register int	nbytes;
int		mode;
{
	register int	c, i;

	if (mode == MODE_BINARY) {
		/*
		 * For binary mode files, no conversion is required.
		 */

		i = write(fileno(fp), ptr, nbytes);
		if (i != nbytes)
			err_dump("write error to local file, i = %d", i);

	} else if (mode == MODE_ASCII) {
		/*
		 * For files that are transferred in netascii, we must
		 * perform the following conversions:
		 *
		 *	CR,LF             ->  newline = '\n'
		 *	CR,NULL           ->  CR      = '\r'
		 *	CR,anything_else  ->  undefined (we don't allow this)
		 *
		 * Note that we have to use the global "lastcr" to remember
		 * if the last character was a carriage-return or not,
		 * since if the last character of a buffer is a CR, we have
		 * to remember that when we're called for the next buffer.
		 */

		for (i = 0; i < nbytes; i++) {
			c = *ptr++;
			if (lastcr) {
				if (c == '\n')
					c = '\n';
				else if (c == '\0')
					c = '\r';
				else
					err_dump("CR followed by 0x%02x", c);
				lastcr = 0;

			} else if (c == '\r') {
				lastcr = 1;
				continue;	/* get next character */
			}

			if (putc(c, fp) == EOF)
				err_dump("write error from putc to local file");
		}
	} else
		err_dump("unknown MODE value");
}
