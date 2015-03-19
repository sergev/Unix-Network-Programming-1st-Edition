/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1983 Regents of the University of California.\n\
 All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)rmt.c	5.4 (Berkeley) 6/29/88";
#endif /* not lint */

/*
 * rmt daemon.
 */

#include	<stdio.h>
#include	<sgtty.h>
#include	<sys/types.h>
#include	<sys/socket.h>
#include	<sys/mtio.h>
#include	<errno.h>
extern int	errno;

int	tapefd = -1;		/* file descriptor for tape device */

char	*record = NULL;		/* pointer to our malloc'ed buffer */

#define	MAXSTRING	64
char	device[MAXSTRING];	/* device/filename to open */
char	count[MAXSTRING];	/* count for read(2) and write(2) and others */
char	offset[MAXSTRING];	/* offset for lseek(2) */
char	whence[MAXSTRING];	/* whence for lseek(2) */
char	mode[MAXSTRING];	/* mode for open(2) */
char	op[MAXSTRING];		/* operation for mt ioctl */

char	respstr[BUFSIZ];	/* response string we send back to client */

long	lseek();
long	atol();
char	*malloc();
char	*checkbuf();		/* our function, at end of file */

FILE	*debug;
#define	DEBUG(f)	if (debug) fprintf(debug, f)
#define	DEBUG1(f,a)	if (debug) fprintf(debug, f, a)
#define	DEBUG2(f,a1,a2)	if (debug) fprintf(debug, f, a1, a2)

main(argc, argv)
int	argc;
char	**argv;
{
	int		n, i, cc, respval;
	long		lrespval;
	char		c;
	struct mtop	mtop;
	struct mtget	mtget;

	argc--, argv++;
	if (argc > 0) {
		/*
		 * If a command line argument is specified, take that as the
		 * name of a file to write debugging information into.
		 */

		if ( (debug = fopen(*argv, "w")) == NULL)
			exit(1);
		setbuf(debug, NULL);	/* completely unbuffered */
	}

	/*
	 * We communicate with the client on descriptors 0 and 1.
	 * Since we're invoked by rshd, both are set to the stream socket
	 * that inetd accepts from the client.
	 * We read from fd 0 and write to fd 1, so that for testing we can
	 * just invoke this program interactively and it'll work with
	 * stdin and stdout.
	 */
top:
	errno   = 0;		/* Unix errno, clear before every operation */

	if (read(0, &c, 1) != 1)
		exit(0);	/* EOF (normal way to end) or error */

	switch (c) {

	case 'O':						/* open */
		if (tapefd >= 0)
			close(tapefd);	/* implies a close of currently-open
					   device */
		get_string(device);
		get_string(mode);
		DEBUG2("rmtd: O %s %s\n", device, mode);

		if ( (tapefd = open(device, atoi(mode))) < 0)
			resp_ioerr(errno);
		else
			resp_val((long) 0);
		break;

	case 'C':						/* close */
		DEBUG("rmtd: C\n");
		get_string(device);		/* required, but ignored */

		if (close(tapefd) < 0)
			resp_ioerr(errno);
		else
			resp_val((long) 0);

		tapefd = -1;	/* will force any i/o operations to generate
				   an error, until another device is opened */
		break;

	case 'L':						/* lseek */
		get_string(offset);
		get_string(whence);
		DEBUG2("rmtd: L %s %s\n", offset, whence);

		if ( (lrespval = lseek(tapefd, atol(offset), atoi(whence))) < 0)
			resp_ioerr(errno);
		else
			resp_val(lrespval);	/* lseek return value */
		break;

	case 'W':						/* write */
		get_string(count);
		DEBUG1("rmtd: W %s\n", count);

		n = atoi(count);
		record = checkbuf(record, n, SO_RCVBUF);

		/*
		 * We have to loop, to read a record of the specified size
		 * from the socket.
		 */

		for (i = 0; i < n; i += cc) {
			if ( (cc = read(0, &record[i], n - i)) <= 0) {
				DEBUG("rmtd: premature eof\n");
				exit(2);
			}
		}

		/*
		 * Write a single tape record.  Note that we don't respond to
		 * the client until the write(2) system call returns.
		 */

		if ( (respval = write(tapefd, record, n)) < 0)
			resp_ioerr(errno);
		else
			resp_val((long) respval);	/* #bytes written */
		break;

	case 'R':						/* read */
		get_string(count);
		DEBUG1("rmtd: R %s\n", count);

		n = atoi(count);
		record = checkbuf(record, n, SO_SNDBUF);

		if ( (respval = read(tapefd, record, n)) < 0)
			resp_ioerr(errno);
		else {
			resp_val((long) respval);	/* #bytes */
			resp_buff(record, respval);	/* the actual data */
		}
		break;

	case 'I':						/* MT ioctl */
		get_string(op);
		get_string(count);
		DEBUG2("rmtd: I %s %s\n", op, count);

		mtop.mt_op    = atoi(op);
		mtop.mt_count = atoi(count);
		if (ioctl(tapefd, MTIOCTOP, (char *) &mtop) < 0)
			resp_ioerr(errno);
		else
			resp_val((long) mtop.mt_count);
		break;

	case 'S':						/* MT status */
		DEBUG("rmtd: S\n");

		if (ioctl(tapefd, MTIOCGET, (char *) &mtget) < 0)
			resp_ioerr(errno);
		else {
			resp_val((long) sizeof(mtget));
			resp_buff((char *) &mtget, sizeof(mtget));
		}
		break;

	default:
		DEBUG1("rmtd: garbage command %c\n", c);
		exit(3);
	}
	goto top;
}

/*
 * Send a normal response to the client.
 */

resp_val(lval)
long	lval;		/* has to be a long, for lseek() return value */
{
	register int	n;

	DEBUG1("rmtd: A %ld\n", lval);

	sprintf(respstr, "A%ld\n", lval);
	n = strlen(respstr);
	if (write(1, respstr, n) != n) {
		DEBUG("rmtd: resp_val: write error\n");
		exit(5);
	}
}

/*
 * Send a response buffer to the client.
 */

resp_buff(buff, nbytes)
char	*buff;
int	nbytes;
{
	if (write(1, buff, nbytes) != nbytes) {
		DEBUG("rmtd: resp_buff: write error\n");
		exit(6);
	}
}

/*
 * Send an error response to the client.
 * Notice that we send the error string associated with "errno" to the
 * client, not just the errno value, since if it is a different flavor
 * of Unix (or not even Unix at all) the errno values may not be
 * meaningful.
 */

resp_ioerr(errnum)
int	errnum;		/* the Unix errno */
{
	char		msgstr[100];
	extern int	sys_nerr;
	extern char	*sys_errlist[];

	if (errnum > 0 && errnum < sys_nerr)
		sprintf(msgstr, "%s", sys_errlist[errnum]);
	else
		sprintf(msgstr, "errno = %d", errnum);

	DEBUG2("rmtd: E %d (%s)\n", errnum, msgstr);
	sprintf(respstr, "E%d\n%s\n", errnum, msgstr);
	resp_buff(respstr, strlen(respstr));
}

/*
 * Get a string from the command line (the socket).
 */

get_string(bp)
char	*bp;		/* string gets stored here by us */
{
	register int	i;
	register char	*cp;

	cp = bp;
	for (i = 0; i < (MAXSTRING - 1); i++) {
		/*
		 * Read one byte at a time, looking for the newline.
		 * Note that this differs from the rmt(8C) man page.
		 * Commands with multiple arguments (such as 'O') require
		 * a newline between the arguments.  Also, we do not skip
		 * over any leading white space, so the command character
		 * must be immediately followed by the first argument.
		 */

		if (read(0, cp+i, 1) != 1)
			exit(0);

		if (cp[i] == '\n')
			break;
	}
	cp[i] = '\0';
}

/*
 * The following function is called before every record is written or read
 * to or from the tape.  Since we have to read or write every tape record
 * with a single read(2) or write(2) system call, we have to assure
 * we have a buffer that is big enough.
 * What we do is keep track of the largest record we've seen so far, and
 * whenever a larger record is required, we free(3) the old buffer and
 * malloc(3) a new one.
 *
 * Additionally, when we malloc a buffer, we set the socket's buffer size
 * to the new size (or as close to it as possible).
 */

static int	maxrecsize = -1;	/* largest record we've seen so far */

char *			/* return pointer to buffer to use */
checkbuf(ptr, size, option)
char	*ptr;		/* pointer to current buffer */
int	size;		/* size of current buffer */
int	option;		/* for setsockopt: SO_SNDBUF or SO_RCVBUF */
{

	if (size <= maxrecsize)
		return(ptr);		/* current buffer is big enough */

	if (ptr != NULL)
		free(ptr);		/* first free the existing buffer */
					/* then malloc a new buffer */
	if ( (ptr = malloc(size)) == NULL) {
		DEBUG("rmtd: cannot allocate buffer space\n");
		exit(4);
	}

	maxrecsize = size;		/* remember new buffer size */

	while ((size > 1024) &&
	       (setsockopt(0, SOL_SOCKET, option, (char *) &size,
				sizeof(size)) < 0))
		size -= 1024;

	return(ptr);		/* return pointer to the new buffer */
}
