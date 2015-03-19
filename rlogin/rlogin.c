/*
 * Copyright (c) 1983 The Regents of the University of California.
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
"@(#) Copyright (c) 1983 The Regents of the University of California.\n\
 All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)rlogin.c	5.12 (Berkeley) 9/19/88";
#endif /* not lint */

/*
 * rlogin - remote login client.
 */

#include <sys/param.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>

#include <netinet/in.h>

#include <stdio.h>
#include <sgtty.h>
#include <pwd.h>
#include <signal.h>
#include <setjmp.h>
#include <netdb.h>
#include <errno.h>
extern int	errno;

/*
 * The server sends us a TIOCPKT_WINDOW notification when it starts up.
 * The value for this (0x80) can't overlap the kernel defined TIOCKPT_xxx
 * values.
 */

#ifndef	TIOCPKT_WINDOW
#define	TIOCPKT_WINDOW 0x80
#endif

#ifndef	SIGUSR1
#define	SIGUSR1 30	/* concession to sun */
#endif

char		*index(), *rindex(), *malloc(), *getenv(), *strcat(), *strcpy();
struct passwd	*getpwuid();
char		*name;
int		sockfd;			/* socket to server */

char		escchar = '~';		/* can be changed with -e flag */
int		eight;			/* can be changed with -8 flag */
int		litout;			/* can be changed with -L flag */

char		*speeds[] = {
	"0", "50", "75", "110", "134", "150", "200", "300",
	"600", "1200", "1800", "2400", "4800", "9600", "19200", "38400"
};
char		term[256] = "network";
int		dosigwinch = 0;		/* set to 1 if the server supports
					   our window-size-change protocol */

#ifndef	sigmask
#define	sigmask(m)	(1 << ((m)-1))
#endif

#ifdef sun
struct winsize {
	unsigned short	ws_row;
	unsigned short	ws_col;
	unsigned short	ws_xpixel;
	unsigned short	ws_ypixel;
};
#endif

struct winsize	currwinsize;		/* current size of window */

int	sigpipe_parent();		/* our signal handlers */
int	sigwinch_parent();
int	sigcld_parent();
int	sigurg_parent();
int	sigusr1_parent();
int	sigurg_child();

/*
 * The following routine provides compatibility (such as it is)
 * between 4.2BSD Suns and others.  Suns have only a `ttysize',
 * so we convert it to a winsize.
 */

#ifdef sun

int
get_window_size(fd, wp)
int		fd;
struct winsize	*wp;
{
	struct ttysize	ts;
	int		error;

	if ( (error = ioctl(0, TIOCGSIZE, &ts)) != 0)
		return(error);
	wp->ws_row    = ts.ts_lines;
	wp->ws_col    = ts.ts_cols;
	wp->ws_xpixel = 0;
	wp->ws_ypixel = 0;
	return(0);
}
#else
#define	get_window_size(fd, wp)		ioctl(fd, TIOCGWINSZ, wp)
#endif	/* sun */

main(argc, argv)
int	argc;
char	**argv;
{
	char		*host, *cp;
	struct sgttyb	ttyb;
	struct passwd	*pwd;
	struct servent	*sp;
	int		uid, options = 0, oldsigmask;
	int		on = 1;

	if ( (host = rindex(argv[0], '/')) != NULL)
		host++;
	else
		host = argv[0];
	argv++, --argc;
	if (strcmp(host, "rlogin") == 0)
		host = *argv++, --argc;
another:
	if (argc > 0 && strcmp(*argv, "-d") == 0) {
		/*
		 * Turn on the debug option for the socket.
		 */

		argv++, argc--;
		options |= SO_DEBUG;
		goto another;
	}

	if (argc > 0 && strcmp(*argv, "-l") == 0) {
		/*
		 * Specify the server-user-name, instead of using the
		 * name of the person invoking us.
		 */

		argv++, argc--;
		if (argc == 0)
			goto usage;
		name = *argv++; argc--;
		goto another;
	}

	if (argc > 0 && strncmp(*argv, "-e", 2) == 0) {
		/*
		 * Specify an escape character, instead of the default tilde.
		 */

		escchar = argv[0][2];
		argv++, argc--;
		goto another;
	}

	if (argc > 0 && strcmp(*argv, "-8") == 0) {
		/*
		 * 8-bit input.  Specifying this forces us to use RAW mode
		 * input from the user's terminal.  Also, in this mode we
		 * won't perform any local flow control.
		 */

		eight = 1;
		argv++, argc--;
		goto another;
	}

	if (argc > 0 && strcmp(*argv, "-L") == 0) {
		/*
		 * 8-bit output.  Causes us to set the LLITOUT flag,
		 * which tells the line discpline: no output translations.
		 */

		litout = 1;
		argv++, argc--;
		goto another;
	}

	if (host == NULL)
		goto usage;
	if (argc > 0)
		goto usage;	/* too many command line arguments */

	/*
	 * Get the name of the user invoking us: the client-user-name.
	 */

	if ( (pwd = getpwuid(getuid())) == NULL) {
		fputs("Who are you?\n", stderr);
		exit(1);
	}

	/*
	 * Get the name of the server we connect to.
	 */

	if ( (sp = getservbyname("login", "tcp")) == NULL) {
		fputs("rlogin: login/tcp: unknown service\n", stderr);
		exit(2);
	}

	/*
	 * Get the name of the terminal from the environment.
	 * Also get the terminal's speed.  Both the name and
	 * the speed are passed to the server as the "cmd"
	 * argument of the rcmd() function.  This is something
	 * like "vt100/9600".
	 */

	if ( (cp = getenv("TERM")) != NULL)
		strcpy(term, cp);
	if (ioctl(0, TIOCGETP, &ttyb) == 0) {
		strcat(term, "/");
		strcat(term, speeds[ttyb.sg_ospeed]);
	}

	get_window_size(0, &currwinsize);
	signal(SIGPIPE, sigpipe_parent);

	/*
	 * Block the SIGURG and SIGUSR1 signals.  These will be handled
	 * by the parent and the child after the fork.
	 */

	oldsigmask = sigblock(sigmask(SIGURG) | sigmask(SIGUSR1));

	/*
	 * Use rcmd() to connect to the server.  Note that even though
	 * we're using rcmd, we specify the port number of the rlogin
	 * server, not the rshd server.  We also pass the terminal-type/speed
	 * as the "command" argument, but the server knows what it is.
	 */

        sockfd = rcmd(&host, sp->s_port, pwd->pw_name,
				name ? name : pwd->pw_name, term, (int *) 0);
        if (sockfd < 0)
                exit(1);

	if ((options & SO_DEBUG) &&
	    setsockopt(sockfd, SOL_SOCKET, SO_DEBUG, &on, sizeof(on)) < 0)
		perror("rlogin: setsockopt (SO_DEBUG)");

	/*
	 * Now change to the real user ID.  We have to be set-user-ID root
	 * to get the privileged port that rcmd() uses,
	 * however we now want to run as the real user who invoked us.
	 */

	uid = getuid();
	if (setuid(uid) < 0) {
		perror("rlogin: setuid");
		exit(1);
	}

	doit(oldsigmask);
	/*NOTREACHED*/
usage:
	fputs("usage: rlogin host [ -ex ] [ -l username ] [ -8 ] [ -L ]\n",
		stderr);
	exit(1);
}

int	childpid;

/*
 * tty flags.  Refer to tty(4) for all the details.
 */

int		defflags;	/* the sg_flags word from the sgttyb struct */
int		tabflag;	/* the two tab bits from the sg_flags word */
int		deflflags;
char		deferase;	/* client's erase character */
char		defkill;	/* client's kill character */
struct tchars	deftc;
struct ltchars	defltc;

/*
 * If you set one of the special terminal characters to -1, that effectively
 * disables the line discipline from processing that special character.
 * We initialize the following two structures to do this.  However, the
 * code below replaces the -1 entries for "stop-output" and "start-output"
 * with the actual values of these two characters (such as ^Q/^S).
 * This way, we can use CBREAK mode but only have the line discipline do
 * flow control.  All other special characters are ignored by our end and
 * passed to the server's line discipline.
 */

struct tchars	notc =	{ -1, -1, -1, -1, -1, -1 };
				/* disables all the tchars: interrupt, quit,
				   stop-output, start-output, EOF */
struct ltchars	noltc =	{ -1, -1, -1, -1, -1, -1 };
				/* disables all ltchars: suspend,
				   delayed-suspend, reprint-line, flush,
				   word-erase, literal-next */

doit(oldsigmask)
int	oldsigmask;		/* mask of blocked signals */
{
	int		exit();
	struct sgttyb	sb;

	ioctl(0, TIOCGETP, (char *) &sb);	/* get the basic modes */
	defflags  = sb.sg_flags;
	tabflag   = defflags & TBDELAY;		/* save the 2 tab bits */
	defflags &= ECHO | CRMOD;
	deferase  = sb.sg_erase;
	defkill   = sb.sg_kill;

	ioctl(0, TIOCLGET, (char *) &deflflags);

	ioctl(0, TIOCGETC, (char *) &deftc);
	notc.t_startc = deftc.t_startc;		/* replace -1 with start char */
	notc.t_stopc  = deftc.t_stopc;		/* replace -1 with stop char */

	ioctl(0, TIOCGLTC, (char *) &defltc);

	signal(SIGINT, SIG_IGN);
	setsignal(SIGHUP, exit);	/* HUP or QUIT go straight to exit() */
	setsignal(SIGQUIT, exit);

	if ( (childpid = fork()) < 0) {
		perror("rlogin: fork");
		done(1);
	}
	if (childpid == 0) {			/* child process == reader */
		tty_mode(1);
		if (reader(oldsigmask) == 0) {
			/*
			 * If the reader() returns 0, the socket to the
			 * server returned an EOF, meaning the client
			 * logged out of the remote system.
			 * This is the normal termination.
			 */

			prf("Connection closed.");
			exit(0);
		}

		/*
		 * If the reader() returns nonzero, the socket to the
		 * server returned an error.  Something went wrong.
		 */

		sleep(1);
		prf("\007Connection closed.");	/* 007 = ASCII bell */
		exit(3);
	}

	/*
	 * Parent process == writer.
	 *
	 * We may still own the socket, and may have a pending SIGURG
	 * (or might receive one soon) that we really want to send to
	 * the reader.  Set a trap that copies such signals to
	 * the child.  Once the two signal handlers are installed,
	 * reset the signal mask to what it was before the fork.
	 */

	signal(SIGURG, sigurg_parent);
	signal(SIGUSR1, sigusr1_parent);
	sigsetmask(oldsigmask);		/* reenables SIGURG and SIGUSR1 */

	signal(SIGCHLD, sigcld_parent);

	writer();

	/*
	 * If the writer returns, it means the user entered "~." on the
	 * terminal.  In this case we terminate and the server will
	 * eventually get an EOF on its end of the network connection.
	 * This should cause the server to log you out on the remote system.
	 */

	prf("Closed connection.");
	done(0);
}

/*
 * Enable a signal handler, unless the signal is already being ignored.
 * This function is called before the fork(), for SIGHUP and SIGQUIT.
 */

setsignal(sig, action)
int	sig;
int	(*action)();
{
	register int	omask;

	omask = sigblock(sigmask(sig));		/* block the signal */

	if (signal(sig, action) == SIG_IGN)
		signal(sig, SIG_IGN);

	sigsetmask(omask);			/* reset the signal mask */
}

/*
 * This function is called by the parent:
 *	(1) at the end (user terminates the client end);
 *	(2) SIGCLD signal - the sigcld_parent() function;
 *	(3) SIGPIPE signal - the connection has dropped.
 *
 * We send the child a SIGKILL signal, which it can't ignore, then
 * wait for it to terminate.
 */

done(status)
int	status;		/* exit() status */
{
	int	w;

	tty_mode(0);	/* restore the user's terminal mode */

	if (childpid > 0) {
		signal(SIGCHLD, SIG_DFL);	/* disable signal catcher */

		if (kill(childpid, SIGKILL) >= 0)
			while ((w = wait((union wait *) 0)) > 0 &&
			       w != childpid)
				;
	}
	exit(status);
}

/*
 * Copy SIGURGs to the child process.
 * The parent shouldn't get any SIGURGs, but if it does, just pass
 * them to the child, as it's the child that handles the out-of-band
 * data from the server.
 */

sigurg_parent()
{
	kill(childpid, SIGURG);
}

/*
 * The child sends the parent a SIGUSR1 signal when the child receives
 * the TIOCPKT_WINDOW indicator from the server.  This tells the
 * client to enable the in-band window-changing protocol.
 */

sigusr1_parent()
{
	if (dosigwinch == 0) {		/* first time */
		/*
		 * First time.  Send the initial window sizes to the
		 * server and enable the SIGWINCH signal, so that we pick
		 * up any changes from this point on.
		 */

		sendwindow();
		signal(SIGWINCH, sigwinch_parent);
		dosigwinch = 1;
	}
}

/*
 * SIGCLD signal haldner in parent.
 */

sigcld_parent()
{
	union wait	status;
	register int	pid;

again:
	/*
	 * WNOHANG -> don't block.
	 * WUNTRACED -> tell us about stopped, untraced children.
	 */

	pid = wait3(&status, WNOHANG | WUNTRACED, (struct rusage *) 0);
	if (pid == 0)
		return;		/* no processes wish to report status */

	/*
	 * If the child (reader) dies, just quit.
	 */

	if (pid < 0 || (pid == childpid && WIFSTOPPED(status) == 0))
		done( (int) (status.w_termsig | status.w_retcode) );
	goto again;
}

/*
 * SIGPIPE signal handler.  We're called if the connection drops.
 * This signal happens in the parent, since the signal is sent to the process
 * that writes to the socket (pipe) that has no reader.
 */

sigpipe_parent()
{
	signal(SIGPIPE, SIG_IGN);
	prf("\007Connection closed.");
	done(1);
}

/*******************************************************************************
 *
 * writer main loop: copy standard input (user's terminal) to network.
 *
 * The standard input is in raw mode, however, we look for three special
 * sequences of characters:
 *
 *	~.	terminate;
 *	~^D	terminate;
 *	~^Z	suspend rlogin process;
 *	~^Y	suspend rlogin process, but leave reader alone.
 *
 * This handling of escape sequences isn't perfect, however.  For example,
 * use rlogin, then run the vi editor on the remote system.  Enter return,
 * then tilde (vi's convert-case-of-character command), then dot (vi's redo
 * last command).  Voila, you're logged out.
 */

writer()
{
	char		c;
	register	n;
	register	bol = 1;               /* beginning of line */
	register	local = 0;

	for ( ; ; ) {
		/*
		 * Since we have to look at every character entered by the
		 * user, we read the standard input one-character-at-a-time.
		 * For human input, this isn't too bad.
		 */

		n = read(0, &c, 1);
		if (n <= 0) {
			if (n < 0 && errno == EINTR)
				continue;
			break;
		}

		/*
		 * If we're at the beginning of the line and recognize
		 * the escape character, then we echo the next character
		 * locally.  If the command character is doubled, for example
		 * if you enter ~~. at the beginning of a line, nothing
		 * is echoed locally and ~. is sent to the server.
		 */

		if (bol) {
			bol = 0;
			if (c == escchar) {
				local = 1;	/* local echo next char */
				continue;	/* next iteration of for-loop */
			}

		} else if (local) {
			/*
			 * The previous character (the first character of
			 * a line) was the escape character.  Look at the
			 * second character of the line and determine if
			 * something special should happen.
			 */

			local = 0;

			if (c == '.' || c == deftc.t_eofc) {
				/*
				 * A tilde-period or tilde-EOF terminates
				 * the parent.  Echo the period or EOF
				 * then stop.
				 */

				echo(c);
				break;		/* breaks out of for-loop */
			}

			if (c == defltc.t_suspc || c == defltc.t_dsuspc) {
				/*
				 * A tilde-^Z or tilde-^Y stops the parent
				 * process.
				 */

				bol = 1;
				echo(c);

				stop(c); /* returns only when we're continued */

				continue;	/* next iteration of for-loop */
			}

			/*
			 * If the input was tilde-someothercharacter,
			 * then we have to write both the tilde and the
			 * other character to the network.
			 */

			if (c != escchar)
				if (write(sockfd, &escchar, 1) != 1) {
					prf("line gone");
					break;
				}
		}

		if (write(sockfd, &c, 1) != 1) {
			prf("line gone");
			break;
		}

		/*
		 * Set a flag if by looking at the current character
		 * we think the next character is going to be the first
		 * character of a line.  This ain't perfect.
		 */

		bol = (c == defkill) ||		/* kill char, such as ^U */
		      (c == deftc.t_eofc) ||	/* EOF char, such as ^D */
		      (c == deftc.t_intrc) ||	/* interrupt, such as ^C */
		      (c == defltc.t_suspc) ||	/* suspend job, such as ^Z */
		      (c == '\r') ||		/* carriage-return */
		      (c == '\n');		/* newline */
	}
}

/*
 * Echo a character on the standard output (the user's terminal).
 * This is called only by the writer() function above to handle the
 * escape characters that we echo.
 */

echo(c)
register char	c;
{
	char		buf[8];
	register char	*p = buf;

	*p++ = escchar;		/* print the escape character first */

	c &= 0177;
	if (c < 040) {
		/*
		 * Echo ASCII control characters as a caret, followed
		 * by the upper case character.
		 */

		*p++ = '^';
		*p++ = c + '@';

	} else if (c == 0177) {		/* ASCII DEL character */
		*p++ = '^';
		*p++ = '?';

	} else
		*p++ = c;

	*p++ = '\r';	/* need a return-linefeed, since it's in raw mode */
	*p++ = '\n';

	write(1, buf, p - buf);
}

/*
 * Stop the parent process (job control).
 * If the character entered by the user is the "stop process" (^Z) character,
 * then we send the SIGTSTP signal to both ourself and the reader (all the
 * processes in the sending processes process group).  When this happens,
 * anything sent by the server to us will be buffered by the network
 * until the reader starts up again and reads it.
 * However, if the character is the "delayed stop process" (^Y) character,
 * then we stop only ourself and not the reader.  This way, the reader
 * continues outputting any data that it receives from the server.
 */

stop(cmdc)
char	cmdc;
{
	tty_mode(0);		/* first reset the terminal mode to normal */

	signal(SIGCHLD, SIG_IGN);  /* ignore SIGCLD in case child stops too */

	kill( (cmdc == defltc.t_suspc) ? 0 : getpid() , SIGTSTP);

		/* resumes here when we're continued by user */
	signal(SIGCHLD, sigcld_parent);
	tty_mode(1);		/* reset terminal back to raw mode */

	sigwinch_parent();	/* see if the window size has changed */
}

/*
 * SIGWINCH signal handler.
 * We're also called above, after we've been resumed after being stopped.
 * We only send a window size message to the server if the size has changed.
 * Note that we use the flag "dosigwinch" to indicate if the server supports
 * our window-size-change protocol.  If the server doesn't tell us that
 * it supports it (see sigusr1_parent() above), we'll never send it.
 */

sigwinch_parent()
{
	struct winsize	ws;

	if (dosigwinch && (get_window_size(0, &ws) == 0) &&
	    (bcmp((char *) &ws, (char *) &currwinsize,
						sizeof(struct winsize)) != 0)) {
		currwinsize = ws;	/* store new size for next time */
		sendwindow();		/* and tell the server */
	}
}

/*
 * Send the window size to the server via the magic escape.
 * Note that we send the 4 unsigned shorts in the structure in network byte
 * order, as it's possible to be running the client and server on systems
 * with different byte orders (a VAX and a Sun, for example).
 */

sendwindow()
{
	char			obuf[4 + sizeof(struct winsize)];
	register struct winsize	*wp;

	wp = (struct winsize *)(obuf + 4);

	obuf[0] = 0377;		/* these 4 bytes are the magic sequence */
	obuf[1] = 0377;
	obuf[2] = 's';
	obuf[3] = 's';

	wp->ws_row    = htons(currwinsize.ws_row);
	wp->ws_col    = htons(currwinsize.ws_col);
	wp->ws_xpixel = htons(currwinsize.ws_xpixel);
	wp->ws_ypixel = htons(currwinsize.ws_ypixel);

	write(sockfd, obuf, sizeof(obuf));
}

/*******************************************************************************
 *
 * reader main loop: copy network to standard output (user's terminal).
 */

char	rcvbuf[8 * 1024];	/* read into here from network */
int	rcvcnt;			/* amount of data in rvcbuf[] */
int	rcvstate;		/* READING or WRITING: so sigurg_child()
				   knows whether a read or write system
				   call was interrupted */
int	parentpid;		/* parent pid, from the fork */
jmp_buf	rcvtop;			/* setjmp/longjmp buffer */

#define	READING	1		/* values for rcvstate */
#define	WRITING	2

reader(oldsigmask)
int	oldsigmask;	/* signal mask from parent */
{
#if !defined(BSD) || BSD < 43
	int	pid = -getpid();
#else
	int	pid = getpid();
#endif
	int	n, remaining;
	char	*bufp = rcvbuf;

	signal(SIGTTOU, SIG_IGN);
	signal(SIGURG, sigurg_child);	/* out-of-band data from server */
	fcntl(sockfd, F_SETOWN, pid);	/* to receive SIGURG signals */

	parentpid = getppid();		/* for SIGUSR1 signal at beginning */

	setjmp(rcvtop);			/* see the longjmps in sigurg_child() */

	sigsetmask(oldsigmask);		/* reset signal mask */
					/* reenables SIGURG and SIGUSR1 */

	for ( ; ; ) {
		/*
		 * Reader main loop - read as much as we can from
		 * the network and write it to standard output.
		 */

		while ( (remaining = rcvcnt - (bufp - rcvbuf)) > 0) {
			/*
			 * While there's data in the buffer to write,
			 * write it to the standard output.
			 */

			rcvstate = WRITING;
			if ( (n = write(1, bufp, remaining)) < 0) {
				if (errno != EINTR)
					return(-1);
				continue;
			}
			bufp += n;	/* incr pointer past what we wrote */
		}

		/*
		 * There's nothing in our buffer to write, so read from
		 * the network.
		 */

		bufp = rcvbuf;		/* ptr to start of buffer */
		rcvcnt = 0;		/* #bytes in buffer */
		rcvstate = READING;

		rcvcnt = read(sockfd, rcvbuf, sizeof(rcvbuf));
		if (rcvcnt == 0)
			return(0);	/* user logged out from remote system */
		if (rcvcnt < 0) {
			if (errno == EINTR)
				continue;
			perror("read");
			return(-1);
		}
	}
}

/*
 * This is the SIGURG signal handler in the child.  Here we process
 * the out-of-band signals that arrive from the server.
 */

sigurg_child()
{
	int		flushflag, atoobmark, n, rcvd;
	char		waste[BUFSIZ], ctlbyte;
	struct sgttyb	sb;

	rcvd = 0;
	while (recv(sockfd, &ctlbyte, 1, MSG_OOB) < 0) {
		switch (errno) {

		case EWOULDBLOCK:
			/*
			 * The Urgent data is not here yet.
			 * It may not be possible to send it yet if we are
			 * blocked for output and our input buffer is full.
			 *
			 * First try to read as much as the receive buffer
			 * has room for.  Note that neither of the reads
			 * below will go past the OOB mark.
			 */

			if (rcvcnt < sizeof(rcvbuf)) {
				n = read(sockfd, rcvbuf + rcvcnt,
						sizeof(rcvbuf) - rcvcnt);
				if (n <= 0)
					return;

				rcvd += n;	/* remember how much we read */

			} else {
				/*
				 * The receive buffer is currently full.
				 * We have no choice but to read into
				 * our wastebasket.
				 */

				n = read(sockfd, waste, sizeof(waste));
				if (n <= 0)
					return;
			}
			continue;	/* try to read to OOB byte again */

		default:
			return;
		}
	}

	/*
	 * Note that in the TIOCPKT mode, any number of the control
	 * bits may be on in the control byte, so we have to test
	 * for all the ones we're interested in.
	 */

	if (ctlbyte & TIOCPKT_WINDOW) {
		/*
		 * We get this control byte from the server after it has
		 * started.  It means that the server is started and
		 * it needs to know the current window size.  We send
		 * the SIGUSR1 signal to the parent, as it is the
		 * parent who must send the window size to the server.
		 */

		kill(parentpid, SIGUSR1);
	}

	if (!eight && (ctlbyte & TIOCPKT_NOSTOP)) {
		/*
		 * Either the server is not using ^S/^Q or the server is
		 * in raw mode.  We must set the user's terminal to
		 * raw mode.  This disables flow control on the client system.
		 */

		ioctl(0, TIOCGETP, (char *) &sb);
		sb.sg_flags &= ~CBREAK;			/* CBREAK off */
		sb.sg_flags |= RAW;			/* RAW on */
		ioctl(0, TIOCSETN, (char *) &sb);	/* doesn't delay */

		notc.t_stopc  = -1;			/* no stop char */
		notc.t_startc = -1;			/* no start char */
		ioctl(0, TIOCSETC, (char *) &notc);
	}

	if (!eight && (ctlbyte & TIOCPKT_DOSTOP)) {
		/*
		 * The server is using ^S/^Q and it's not in raw mode,
		 * so we can do flow control on the client system.
		 */

		ioctl(0, TIOCGETP, (char *) &sb);
		sb.sg_flags &= ~RAW;			/* RAW off */
		sb.sg_flags |= CBREAK;			/* CBREAK on */
		ioctl(0, TIOCSETN, (char *) &sb);

		notc.t_stopc  = deftc.t_stopc;		/* enable stop */
		notc.t_startc = deftc.t_startc;		/* enable start */
		ioctl(0, TIOCSETC, (char *) &notc);
	}

	if (ctlbyte & TIOCPKT_FLUSHWRITE) {
		/*
		 * The terminal output queue on the server was flushed.
		 * First we flush our terminal output queue (the output
		 * queue for the user's terminal).
		 */

		flushflag = FWRITE;	/* flush output only, not input */
		ioctl(1, TIOCFLUSH, (char *) &flushflag);

		/*
		 * Now we continue reading from the socket, throwing
		 * away all the data until we reach the out-of-band mark.
		 */

		for ( ; ; ) {
			if (ioctl(sockfd, SIOCATMARK, &atoobmark) < 0) {
				perror("ioctl SIOCATMARK error");
				break;
			}
			if (atoobmark)
				break;	/* we're at the oob mark */

			if ( (n = read(sockfd, waste, sizeof(waste))) <= 0)
				break;
		}

		/*
		 * We don't want any pending data that we've already read
		 * into the receive buffer to be output, so clear the receive
		 * buffer (i.e., just set rcvcnt = 0).
		 * Also, if we were hanging on a write to standard output
		 * when interrupted, we don't want it to restart, so we
		 * longjmp back to the top of the loop.
		 * If we were reading, we want to restart it anyway.
		 */

		rcvcnt = 0;
		longjmp(rcvtop, 1);	/* back to the setjmp */
					/* the arg of 1 isn't used */
	}

	/*
	 * If we read data into the receive buffer above (so that we
	 * could read the OOB byte) and if we we're interrupted during
	 * a read, then longjmp to the top of the loop to write the
	 * data that was received.
	 * Don't abort a pending write, however, or we won't know how
	 * much was written.
	 */

	if (rcvd > 0 && rcvstate == READING)
		longjmp(rcvtop, 1);

	return;		/* from the signal handler; probably causes an EINTR */
}

/*
 * Set the terminal mode.  This function affects the user's terminal.
 * We're called by both the parent and child.
 */

tty_mode(mode)
int	mode;		/* 0 -> reset to normal; 1 -> set for rlogin */
{
	struct tchars	*tcptr;
	struct ltchars	*ltcptr;
	struct sgttyb	sb;		/* basic modes */
	int		lflags;		/* local mode word */

	ioctl(0, TIOCGETP, (char *) &sb);
	ioctl(0, TIOCLGET, (char *) &lflags);

	switch (mode) {

	case 0:
		/*
		 * This is called by the parent when it's done to reset
		 * the terminal state to how it found it.
		 * The parent also calls this to reset the terminal state
		 * before stopping itself with job control.
		 */

		sb.sg_flags &= ~(CBREAK | RAW | TBDELAY);
		sb.sg_flags |= defflags | tabflag;

		tcptr = &deftc;		/* restore all special chars */
		ltcptr = &defltc;
		sb.sg_kill = defkill;
		sb.sg_erase = deferase;
		lflags = deflflags;
		break;

	case 1:
		/*
		 * This is called by the child when it starts, to set the
		 * terminal to a raw mode.  Actually, we default to CBREAK
		 * unless the -8 flag was specified (8-bit input) in which
		 * case we have to use RAW mode.
		 * The parent also calls this when resumed, after being
		 * stopped by job control.
		 */

		sb.sg_flags |= (eight ? RAW : CBREAK);
		sb.sg_flags &= ~defflags;
			/* preserve tab delays, but turn off XTABS */
		if ((sb.sg_flags & TBDELAY) == XTABS)
			sb.sg_flags &= ~TBDELAY;

		tcptr = &notc;		/* disable all special chars */
		ltcptr = &noltc;
		sb.sg_kill = -1;
		sb.sg_erase = -1;
		if (litout)
			lflags |= LLITOUT;	/* no output translations */
		break;

	default:
		return;
	}

	ioctl(0, TIOCSLTC, (char *) ltcptr);
	ioctl(0, TIOCSETC, (char *) tcptr);
	ioctl(0, TIOCSETN, (char *) &sb);
	ioctl(0, TIOCLSET, (char *) &lflags);
}

/*
 * Fatal error.
 */

prf(str)
char	*str;
{
	fputs(str, stderr);
	fputs("\r\n", stderr);	/* return & newline, in case raw mode */
}
