/*
 * Copyright (c) 1983, 1988 The Regents of the University of California.
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
"@(#) Copyright (c) 1983, 1988 The Regents of the University of California.\n\
 All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)rlogind.c	5.22.1.6 (Berkeley) 2/7/89";
#endif /* not lint */

/*
 * remote login server:  the following data is sent across the network
 * connection by the rcmd() function that the rlogin client uses:
 *	\0  (there is no auxiliary port used by the client and server)
 *	client-user-name\0
 *	server-user-name\0
 *	terminal-type/speed\0
 *	data
 *
 * Define OLD_LOGIN for compatibility with the 4.2BSD and 4.3BSD /bin/login.
 * If this isn't defined, a newer protocol is used whereby rlogind does
 * the user verification.  This only works if your /bin/login supports the
 * -f and -h flags.  This newer version of login is on the Berkeley
 * Networking Release 1.0 tape.
 */

#ifndef	OLD_LOGIN
#define	NEW_LOGIN	/* make the #ifdefs easier to understand */
#endif

#include <stdio.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/file.h>

#include <netinet/in.h>

#include <pwd.h>
#include <signal.h>
#include <sgtty.h>
#include <stdio.h>
#include <netdb.h>
#include <syslog.h>
#include <strings.h>
#include <errno.h>
extern int	errno;

/*
 * We send a TIOCPKT_WINDOW notification to the client when we start up.
 * This tells the client that we support the window-size-change protocol.
 * The value for this (0x80) can't overlap the kernel defined TIOCKPT_xxx
 * values.
 */

#ifndef TIOCPKT_WINDOW
#define TIOCPKT_WINDOW 0x80
#endif

char		*env[2];		/* the environment we build */
static char	term[64] = "TERM=";
#define	ENVSIZE	(sizeof("TERM=")-1)	/* skip null for concatenation */

#define	NMAX 30
char		cliuname[NMAX+1];	/* user name on client's host */
char		servuname[NMAX+1];	/* user name on server's host */

int		keepalive = 1;		/* set to 0 with -n flag */

#define	SUPERUSER(pwd)	((pwd)->pw_uid == 0)

int		reapchild();
struct passwd	*getpwnam(), *pwd;
char		*malloc();
int		one = 1;		/* for setsockopt() */

main(argc, argv)
int	argc;
char	**argv;
{
	extern int		opterr, optind;
	int			ch, addrlen;
	struct sockaddr_in	cli_addr;

	openlog("rlogind", LOG_PID | LOG_CONS, LOG_AUTH);

	opterr = 0;
	while ( (ch = getopt(argc, argv, "ln")) != EOF)
		switch (ch) {
		case 'l':
#ifdef	NEW_LOGIN
		    {
			extern int	_check_rhosts_file;

			_check_rhosts_file = 0;	/* don't check .rhosts file */
		    }
#endif
			break;

		case 'n':
			keepalive = 0;		/* don't enable SO_KEEPALIVE */
			break;

		case '?':
		default:
			syslog(LOG_ERR, "usage: rlogind [-l] [-n]");
			break;
		}

	argc -= optind;
	argv += optind;

	/*
	 * We assume we're invoked by inetd, so the socket that the connection
	 * is on, is open on descriptors 0, 1 and 2.
	 *
	 * First get the Internet address of the client process.
	 * This is required for all the authentication we perform.
	 */

	addrlen = sizeof(cli_addr);
	if (getpeername(0, &cli_addr, &addrlen) < 0) {
		syslog(LOG_ERR, "Couldn't get peer name of remote host: %m");
		fatalperror("Can't get peer name of host");
	}

	if (keepalive &&
	    setsockopt(0, SOL_SOCKET, SO_KEEPALIVE, (char *) &one,
							       sizeof(one)) < 0)
		syslog(LOG_WARNING, "setsockopt (SO_KEEPALIVE): %m");

	doit(&cli_addr);
}

int		child;
int		cleanup();
char		*line;
extern char	*inet_ntoa();

struct winsize	win = { 0, 0, 0, 0 };

doit(cli_addrp)
struct sockaddr_in	*cli_addrp;	/* client's Internet address */
{
	int			i, masterfd, slavefd, childpid;
#ifdef	NEW_LOGIN
	int			authenticated = 0, hostok = 0;
	char			remotehost[2 * MAXHOSTNAMELEN + 1];
#endif
	register struct hostent	*hp;
	struct hostent		hostent;
	char			c;

	/*
	 * Read the null byte from the client.  This byte is really
	 * written by the rcmd() function as the secondary port number.
	 * However, the rlogin client calls rcmd() specifying a secondary
	 * port of 0, so all that rcmd() sends is the null byte.
	 * We set a timer of 60 seconds to do this read, else we assume
	 * something is wrong.
	 */

	alarm(60);
	read(0, &c, 1);
	if (c != 0)
		exit(1);
	alarm(0);

	/*
	 * Try to look up the client's name, given its internet
	 * address, since we use the name for the authentication.
	 */

	cli_addrp->sin_port = ntohs((u_short)cli_addrp->sin_port);
	hp = gethostbyaddr(&cli_addrp->sin_addr, sizeof(struct in_addr),
							cli_addrp->sin_family);
	if (hp == NULL) {
		/*
		 * Couldn't find the client's name.
		 * Use its dotted-decimal address as its name.
		 */

		hp = &hostent;
		hp->h_name = inet_ntoa(cli_addrp->sin_addr);
#ifdef	NEW_LOGIN
		hostok++;
#endif
	}
#ifdef	NEW_LOGIN
	  else if (local_domain(hp->h_name)) {
		/*
		 * If the name returned by gethostbyaddr() is in our domain,
		 * attempt to verify that we haven't been fooled by someone
		 * in a remote net.  Look up the name and check that this
		 * address corresponds to the name.
		 */

		strncpy(remotehost, hp->h_name, sizeof(remotehost) - 1);
		remotehost[sizeof(remotehost) - 1] = 0;
		if ( (hp = gethostbyname(remotehost)) != NULL) {
			for ( ; hp->h_addr_list[0]; hp->h_addr_list++) {
				if (bcmp(hp->h_addr_list[0],
					 (caddr_t)&cli_addrp->sin_addr,
			    		 sizeof(cli_addrp->sin_addr)) == 0) {
					hostok++;	/* equal, OK */
					break;
				}
			}
		}
	} else
		hostok++;
#endif

	/*
	 * Verify that the client's address is an internet address and
	 * that it was bound to a reserved port.
	 */

	if (cli_addrp->sin_family != AF_INET ||
	    cli_addrp->sin_port >= IPPORT_RESERVED ||
	    cli_addrp->sin_port <  IPPORT_RESERVED/2) {
		syslog(LOG_NOTICE, "Connection from %s on illegal port",
					inet_ntoa(cli_addrp->sin_addr));
		fatal(0, "Permission denied");
	}

#ifdef IP_OPTIONS
    {
	u_char 		optbuf[BUFSIZ/3], *optptr;
	char 		lbuf[BUFSIZ], *lptr;
	int 		optsize, ipproto;
	struct protoent	*ip;

	if ( (ip = getprotobyname("ip")) != NULL)
		ipproto = ip->p_proto;
	else
		ipproto = IPPROTO_IP;

	optsize = sizeof(optbuf);
	if (getsockopt(0, ipproto, IP_OPTIONS, (char *)optbuf, &optsize) == 0 &&
	    optsize != 0) {
		/*
		 * The client has set IP options.  This isn't allowed.
		 * Use syslog() to record the fact.
		 */

		lptr = lbuf;
		optptr = optbuf;
		for ( ; optsize > 0; optptr++, optsize--, lptr += 3)
			sprintf(lptr, " %2.2x", *optptr);
				/* print each option byte as 3 ASCII chars */
		syslog(LOG_NOTICE,
		    "Connection received using IP options (ignored): %s", lbuf);

		/*
		 * Turn off the options.  If this doesn't work, we quit.
		 */

		if (setsockopt(0, ipproto, IP_OPTIONS,
		    			(char *) NULL, &optsize) != 0) {
			syslog(LOG_ERR, "setsockopt IP_OPTIONS NULL: %m");
			exit(1);
		}
	}
    }
#endif	/* IP_OPTIONS */

	/*
	 * Write a null byte back to the client telling it that
	 * everything is OK.  Note that this is different from the
	 * sequence performed by rshd.  The rshd server first reads
	 * the 3 strings on the socket before writing back the null
	 * byte if all is OK.  We can get away with this, since we're
	 * using a full-duplex socket.
	 */

	write(0, "", 1);

#ifdef	NEW_LOGIN
	if (do_rlogin(hp->h_name) == 0) {
		if (hostok)
			authenticated++;
		else
			write(0, "rlogind: Host address mismatch.\r\n",
		     	  sizeof("rlogind: Host address mismatch.\r\n") - 1);
	}
#endif

	/*
	 * Allocate and open a master pseudo-terminal.
	 */

	for (c = 'p'; c <= 's'; c++) {
		struct stat	statbuff;

		line = "/dev/ptyXY";
		line[8] = c;			/* X = [pqrs] */
		line[9] = '0';			/* Y = 0 */

		if (stat(line, &statbuff) < 0)
			break;

		for (i = 0; i < 16; i++) {
			line[9] = "0123456789abcdef"[i];
			if ( (masterfd = open(line, O_RDWR)) > 0)
				goto gotpty;	/* got the master ptr */
		}
	}
	fatal(0, "Out of ptys");
	/*NOTREACHED*/

gotpty:
	ioctl(masterfd, TIOCSWINSZ, &win);	/* set window sizes all to 0 */

	/*
	 * Now open the slave pseudo-terminal corresponding to the
	 * master that we opened above.
	 */

	line[5] = 't';		/* change "/dev/ptyXY" to "/dev/ttyXY" */
	if ( (slavefd = open(line, O_RDWR)) < 0)
		fatalperror(0, line);
	if (fchmod(slavefd, 0))
		fatalperror(0, line);

	/*
	 * The 4.3BSD vhangup() system call does a virtual hangup on the
	 * current control terminal.  It goes through the kernel's tables
	 * and for every reference it finds to the current control terminal,
	 * it revokes that reference (i.e., unconnects any former processes
	 * that may have had this terminal as their control terminal).
	 * vhangup() also sends a SIGHUP to the process group of the control
	 * terminal, so we ignore this signal.
	 */

	signal(SIGHUP, SIG_IGN);
	vhangup();
	signal(SIGHUP, SIG_DFL);

	/*
	 * Now reopen the slave pseudo-terminal again and set it's mode.
	 * This gives us a "clean" control terminal.
	 * line[] contains the string "/dev/ttyXY" which will be used
	 * by the cleanup() function when we're done.
	 */

	if ( (slavefd = open(line, O_RDWR)) < 0)
		fatalperror(0, line);

	setup_term(slavefd);

#ifdef DEBUG
	{
		int	tt;

		if ( (tt = open("/dev/tty", O_RDWR)) > 0) {
			ioctl(tt, TIOCNOTTY, 0);
			close(tt);
		}
	}
#endif

	if ( (childpid = fork()) < 0)
		fatalperror(0, "");

	if (childpid == 0) {
		/*
		 * Child process.  Becomes the login shell for the user.
		 */

		close(0);		/* close socket */
		close(masterfd);	/* close pty master */
		dup2(slavefd, 0);	/* pty slave is 0,1,2 of login shell */
		dup2(slavefd, 1);
		dup2(slavefd, 2);
		close(slavefd);

#ifdef OLD_LOGIN
		/*
		 * Invoke /bin/login with the -r argument, which tells
		 * it was invoked by rlogind.  This causes login to read the
		 * the socket for the client-user-name, the server-user-name
		 * and the terminal-type/speed.  login then calls the
		 * ruserok() function and possibly prompts the client for
		 * their password.
		 */

		execl("/bin/login", "login", "-r", hp->h_name, (char *) 0);

#else /* NEW_LOGIN */
		/*
		 * The -p flag tells login not to destroy the environment.
		 * The -h flag passes the name of the client's system to
		 * login, so it can be placed in the utmp and wtmp entries.
		 * The -f flag says the user has already been authenticated.
		 */

		if (authenticated)
			execl("/bin/login", "login", "-p", "-h", hp->h_name,
				"-f", servuname, (char *) 0);
		else
			execl("/bin/login", "login", "-p", "-h", hp->h_name,
				servuname, (char *) 0);
#endif /* OLD_LOGIN */

		fatalperror(2, "/bin/login");	/* exec error */
		/*NOTREACHED*/
	}

	/*
	 * Parent process.
	 */

	close(slavefd);			/* close slave pty, child uses it */

	ioctl(0, FIONBIO, &one);	/* nonblocking I/O for socket */
	ioctl(masterfd, FIONBIO, &one);	/* nonblocking I/O for master pty */
	ioctl(masterfd, TIOCPKT, &one);	/* BSD pty packet mode */

	signal(SIGTSTP, SIG_IGN);
	signal(SIGCHLD, cleanup);

	setpgrp(0, 0);			/* set our process group to 0 */

	protocol(0, masterfd);		/* this does it all */

	signal(SIGCHLD, SIG_IGN);
	cleanup();
}

/*
 * Define the pty packet-mode control bytes that we're interested in.
 * We ignore any other of the control bytes.
 */

#define	pkcontrol(c)	((c)&(TIOCPKT_FLUSHWRITE|TIOCPKT_NOSTOP|TIOCPKT_DOSTOP))

/*
 * The following byte always gets sent along with the pty packet-mode
 * control byte to the client.  It's initialized to TIOCPKT_WINDOW
 * but this bit gets turned off after the client has sent the first
 * window size.  Thereafter this byte is 0.
 */

char	oobdata[] = {TIOCPKT_WINDOW};

/*
 * Handle an in-band control request from the client.
 * These are signaled by two consecutive magic[] bytes appearing in
 * the data from the client.  The next two bytes in the data stream
 * tell us what type of control message this is.
 * For now, all we handle is a window-size-change.
 *
 * We return the number of bytes that we processed in the buffer, so that
 * the caller can skip over them.
 */

char	magic[2] = { 0377, 0377 };	/* in-band magic cookie */

int
control(pty, cp, n)
int	pty;		/* fd of pty master */
char	*cp;		/* pointer to first two bytes of control sequence */
int	n;
{
	struct winsize	w;

	if (n < 4+sizeof(w) || cp[2] != 's' || cp[3] != 's')
		return (0);

	/*
	 * Once we receive one of these in-band control requests from
	 * the client we know that it received the TIOCPKT_WINDOW
	 * message that we sent it on startup.  We only send this
	 * control byte at the beginning, to tell the client that we
	 * support window-size-changes.  Now we can turn off the
	 * TIOCPKT_WINDOW bit in our control byte.
	 */

	oobdata[0] &= ~TIOCPKT_WINDOW;

	bcopy(cp+4, (char *) &w, sizeof(w));	/* copy into structure */
	w.ws_row    = ntohs(w.ws_row);	/* and change to host byte order */
	w.ws_col    = ntohs(w.ws_col);
	w.ws_xpixel = ntohs(w.ws_xpixel);
	w.ws_ypixel = ntohs(w.ws_ypixel);
	ioctl(pty, TIOCSWINSZ, &w);		/* set the new window size */

	return (4 + sizeof(w));
}

/*
 * rlogin server "protocol" machine.
 *
 * The only condition for which we return to the caller is if we get an
 * error or EOF on the network connection.
 */

protocol(socketfd, masterfd)
int	socketfd;	/* network connection to client */
int	masterfd;	/* master pseudo-terminal */
{
	char		mptyibuf[1024], sockibuf[1024], *mptybptr, *sockbptr;
	register int	mptycc, sockcc;
	int		cc, maxfdp1;
	int		mptymask, sockmask;
	char		cntlbyte;

	mptycc = 0;		/* count of #bytes in buffer */
	sockcc = 0;

	/*
	 * We must ignore SIGTTOU, otherwise we'll stop when we try
	 * and set the slave pty's window size (our controlling tty
	 * is the master pty).
	 */

	signal(SIGTTOU, SIG_IGN);

	/*
	 * Send the TIOCPKT_WINDOW control byte to the client
	 * (as an OOB data byte) telling it that we'll accept
	 * window-size changes.
	 */

	send(socketfd, oobdata, 1, MSG_OOB);

	/*
	 * Set things up for the calls to select().
	 * We cheat and store the file descriptor masks in an int,
	 * knowing that they can't exceed 32 (or something else is wrong).
	 */

	if (socketfd > masterfd)	/* determine max descriptor */
		maxfdp1 = socketfd + 1;
	else
		maxfdp1 = masterfd + 1;

	sockmask = 1 << socketfd;	/* select mask for this descriptor */
	mptymask = 1 << masterfd;

	/*
	 * This loop multiplexes the 2 I/O "streams":
	 *	network input -> sockibuf[]
	 *	                 sockibuf[] -> master pty (input from client)
	 *
	 *	master pty input -> mptyibuf[]
	 *	                    mptyibuf[] -> network (output for client)
	 */

	for ( ; ; ) {
		int	ibits, obits, ebits;

		ibits = 0;
		obits = 0;
		if (sockcc)
			obits |= mptymask;
		else
			ibits |= sockmask;
		if (mptycc >= 0) {
			if (mptycc)
				obits |= sockmask;
			else
				ibits |= mptymask;
		}
		ebits = mptymask;
		if (select(maxfdp1, (fd_set *) &ibits,
				obits ? (fd_set *) &obits : (fd_set *) NULL,
				(fd_set *) &ebits, (struct timeval *) 0) < 0) {
			if (errno == EINTR)
				continue;
			fatalperror(socketfd, "select");
		}

		if (ibits == 0 && obits == 0 && ebits == 0) {
			/* shouldn't happen... */
			sleep(5);
			continue;
		}

		if ((ebits & mptymask) != 0) {
			/*
			 * There is an exceptional condition on the master
			 * pty.  In the pty packet mode, this means there
			 * is a single TIOCPKT_xxx control byte available.
			 * Send that control byte to the client as OOB data.
			 */

			cc = read(masterfd, &cntlbyte, 1);
			if (cc == 1 && pkcontrol(cntlbyte)) {
				cntlbyte |= oobdata[0];
				send(socketfd, &cntlbyte, 1, MSG_OOB);

				if (cntlbyte & TIOCPKT_FLUSHWRITE) {
					/*
					 * If the pty slave flushed its output
					 * queue, then we want to throw away
					 * anything we have in our buffer to
					 * send to the client.
					 */

					mptycc = 0;
					ibits &= ~mptymask;
				}
			}
			/* else could be a packet-mode control byte that
			   we're not interested in */
		}

		if ((ibits & sockmask) != 0) {
			/*
			 * There is input ready on the socket from the client.
			 */

			sockcc = read(socketfd, sockibuf, sizeof(sockibuf));
			if (sockcc < 0 && errno == EWOULDBLOCK)
				sockcc = 0;
			else {
				register char	*ptr;
				int		left, n;

				if (sockcc <= 0)
					break;
				sockbptr = sockibuf;

			top:
				for (ptr = sockibuf; ptr < sockibuf+sockcc-1;
									ptr++) {
					if (ptr[0] == magic[0] &&
					    ptr[1] == magic[1]) {
						/*
						 * We have an in-band control
						 * message.  Process it.  After
						 * we've processed it we have to
						 * move all the remaining data
						 * in the buffer left, and check
						 * for any more in-band control
						 * messages.  Ugh.
						 */

						left = sockcc - (ptr-sockibuf);
						n = control(masterfd, ptr, left);
						if (n) {
						   left -= n;
						   if (left > 0)
						      bcopy(ptr+n, ptr, left);
						   sockcc -= n;
						   goto top; /* n^2 */
						}
					}
				}
				obits |= mptymask;	/* try write */
			}
		}

		if ((obits & mptymask) != 0  &&  sockcc > 0) {
			/*
			 * The master pty is ready to accept data and there
			 * is data from the socket to write to the mpty.
			 * An error from the write is not fatal, since we
			 * set the master pty to nonblocking and it may
			 * not really be ready for writing (see "try write"
			 * comment above).
			 */

			cc = write(masterfd, sockbptr, sockcc);
			if (cc > 0) {
				sockcc -= cc;	/* write succeeded */
				sockbptr += cc;	/* update counter and pointer */
			}
		}

		if ((ibits & mptymask) != 0) {
			/*
			 * There is input from the master pty.  Read it into
			 * the beginning of out "mptyibuf" buffer.
			 */

			mptycc = read(masterfd, mptyibuf, sizeof(mptyibuf));
			mptybptr = mptyibuf;
			if (mptycc < 0 && errno == EWOULDBLOCK)
				mptycc = 0;
			else if (mptycc <= 0)
				break;	/* returns from function; done */
			else if (mptyibuf[0] == 0) {
				/*
				 * If the first byte that we read is a 0, then
				 * there is real data in the buffer for us,
				 * not one of the packet-mode control bytes.
				 */

				mptybptr++;	/* skip over the byte of 0 */
				mptycc--;
				obits |= sockmask;   /* try a write to socket */
			} else {
				/*
				 * It's possible for the master pty to generate
				 * a control byte for us, between the select
				 * above and the read that we just did.
				 */

				if (pkcontrol(mptyibuf[0])) {
				    mptyibuf[0] |= oobdata[0];
				    send(socketfd, &mptyibuf[0], 1, MSG_OOB);
				}
				/* else it has to be one of the packet-mode
				   control bytes that we're not interested in */

				mptycc = 0;	/* there can't be any data after
						   the control byte */
			}
		}

		if ((obits & sockmask) != 0  &&  mptycc > 0) {
			/*
			 * The socket is ready for more output and we have
			 * data from the master pty to send to the client.
			 */

			cc = write(socketfd, mptybptr, mptycc);
			if (cc < 0 && errno == EWOULDBLOCK) {
				/* also shouldn't happen */
				sleep(5);
				continue;
			}
			if (cc > 0) {
				mptycc -= cc;	/* update counter and pointer */
				mptybptr += cc;
			}
		}
	}
}

/*
 * This function is called if a SIGCLD signal occurs.  This means that our
 * child, the login shell that we invoked through /bin/login, terminated.
 * This function is also called at the end of the parent, which only happens
 * if it gets an error or EOF on the network connection to the client.
 */

cleanup()
{
	char	*p;

	/*
	 * Remove the /etc/utmp entry by calling the logout() function.
	 * Then add the terminating entry to the /usr/adm/wtmp file.
	 */

	p = line + 5;		/* p = pointer to "ttyXY" */
	if (logout(p))
		logwtmp(p, "", "");

	chmod(line, 0666);	/* change mode of slave to rw-rw-rw */
	chown(line, 0, 0);	/* change owner=root, group-owner=wheel */

	*p = 'p';		/* change "ttyXY" to "ptyXY" */
	chmod(line, 0666);	/* change mode of master to rw-rw-rw */
	chown(line, 0, 0);	/* change owner=root, group-owner=wheel */

	shutdown(0, 2);		/* close both directions of socket */

	exit(1);
}

/*
 * Send an error message back to the rlogin client.
 * The first byte must be a binary 1, followed by the ASCII
 * error message, followed by a return/newline.
 */

fatal(fd, msg)
int	fd;
char	*msg;
{
	char buf[BUFSIZ];

	buf[0] = 1;
	sprintf(buf + 1, "rlogind: %s.\r\n", msg);
	write(fd, buf, strlen(buf));
	exit(1);
}

/*
 * Fatal error, as above, but include the errno value in the message.
 */

fatalperror(fd, msg)
int	fd;
char	*msg;
{
	char		buf[BUFSIZ];
	extern int	sys_nerr;
	extern char	*sys_errlist[];

	if ((unsigned) errno < sys_nerr)
		sprintf(buf, "%s: %s", msg, sys_errlist[errno]);
	else
		sprintf(buf, "%s: Error %d", msg, errno);

	fatal(fd, buf);
		/* NOTREACHED */
}

#ifdef	OLD_LOGIN

/*
 * Set up the slave pseudo-terminal.
 * This is because the slave becomes standard input, standard output,
 * and standard error of /bin/login.
 * The mode of the slave's pty will be reset again by /bin/login.
 */

setup_term(fd)
int	fd;
{
	struct sgttyb	sgttyb;

	ioctl(fd, TIOCGETP, &sgttyb);
	sgttyb.sg_flags = RAW | ANYP;
			/* raw mode */
			/* accept any parity, send none */
	ioctl(fd, TIOCSETP, &sgttyb);
}

#endif	/* OLD_LOGIN */

#ifdef	NEW_LOGIN

/*
 * The new rlogind does the user authentication here.  In 4.2BSD & 4.3BSD
 * this was done by the login program when invoked with the -r flag.
 */

int			/* return 0 if user validated OK, else -1 on error */
do_rlogin(host)
char	*host;
{
	/*
	 * Read the 3 strings that the rcmd() function wrote to the
	 * socket: the client-user-name, server-user-name and
	 * terminal-type/speed.
	 */

	getstr(cliuname, sizeof(cliuname), "remuser too long");
	getstr(servuname, sizeof(servuname), "locuser too long");
	getstr(term+ENVSIZE, sizeof(term)-ENVSIZE, "Terminal type too long");

	/*
	 * The real-user-ID has to be root since we're invoked by the
	 * inetd daemon.
	 */

	if (getuid())
		return(-1);

	/*
	 * The server-user-name has to correspond to an account on
	 * this system.
	 */

	if ( (pwd = getpwnam(servuname)) == NULL)
		return(-1);

	/*
	 * Call the ruserok() function to authenticate the client.
	 * This function returns 0 if OK, else -1 on error.
	 */

	return( ruserok(host, SUPERUSER(pwd), cliuname, servuname) );
}

/*
 * Read a string from the socket.  Make sure it fits, else fatal error.
 */

getstr(buf, cnt, errmsg)
char	*buf;		/* the string that's read goes into here */
int	cnt;		/* sizeof() the char array */
char	*errmsg;	/* in case error message required */
{
	char	 c;

	do {
		if (read(0, &c, 1) != 1)
			exit(1);		/* error or EOF */
		if (--cnt < 0)
			fatal(1, errmsg);	/* no return */
		*buf++ = c;
	} while (c != 0);		/* null byte terminates the string */
}

extern	char **environ;

char	*speeds[] = {	/* the order *IS* important - see tty(4) */
	"0", "50", "75", "110", "134", "150", "200", "300", "600",
	"1200", "1800", "2400", "4800", "9600", "19200", "38400",
};
#define	NSPEEDS	(sizeof(speeds) / sizeof(speeds[0]))

/*
 * Set up the slave pseudo-terminal device.
 * We take the terminal name that was sent over by the rlogin client,
 * along with the speed.  We set the speed of the slave pty accordingly
 * (since programs such as vi do things differently based on the user's
 * terminal speed) and propagate the terminal type into the initial
 * environment.
 */

setup_term(fd)
int	fd;
{
	register char	*cp, **cpp;
	struct sgttyb	sgttyb;
	char		*speed;

	ioctl(fd, TIOCGETP, &sgttyb);	/* fetch modes for slave pty */

	if ( (cp = index(term, '/')) != NULL) {
		/*
		 * The rlogin client sends a string such as "vt100/9600"
		 * which was stored in the term[] array by do_rlogin().
		 */

		*cp++ = '\0';	/* null terminate the terminal name */
		speed = cp;	/* and get pointer to ASCII speed */

		/*
		 * Assure the ASCII speed is null terminated, in case
		 * it's followed by another slash.  This allows the client
		 * to append additional things to the string, separated
		 * by slashes, even though we don't currently look at them.
		 */

		if ( (cp = index(speed, '/')) != NULL)
			*cp++ = '\0';

		/*
		 * Compare the ASCII speed with the array above, and set
		 * the slave pty speed accordingly.
		 */

		for (cpp = speeds; cpp < &speeds[NSPEEDS]; cpp++) {
			if (strcmp(*cpp, speed) == 0) {
			    sgttyb.sg_ispeed = sgttyb.sg_ospeed = cpp - speeds;
			    break;
			}
		}
	}
	sgttyb.sg_flags = ECHO|CRMOD|ANYP|XTABS;
			/* echo on */
			/* map CR into LF; output LF as CR-LF */
			/* accept any parity, send none */
			/* replace tabs by spaces on output */

	ioctl(fd, TIOCSETP, &sgttyb);

	/*
	 * Initialize the environment that we'll ask /bin/login to
	 * maintain.  /bin/login will then append its variables
	 * (HOME, SHELL, USER, PATH, ...) to this.
	 */

	env[0] = term;		/* the "TERM=..." string */
	env[1] = (char *) 0;	/* one element is all we initialize it with */
	environ = env;		/* stuff it away for our execl of /bin/login */
}

/*
 * Check whether the specified host is in our local domain, as determined
 * by the part of the name following the first period, in its name and in ours.
 * If either name is unqualified (contains no period), assume that the host
 * is local, as it will be interpreted as such.
 */

int				/* return 1 if local domain, else return 0 */
local_domain(host)
char	*host;
{
	register char	*ptr1, *ptr2;
	char		localhost[MAXHOSTNAMELEN];

	if ( (ptr1 = index(host, '.')) == NULL)
		return(1);		/* no period in remote host name */

	gethostname(localhost, sizeof(localhost));
	if ( (ptr2 = index(localhost, '.')) == NULL)
		return(1);		/* no period in local host name */

	/*
	 * Both host names contain a period.  Now compare both names,
	 * starting with the first period in each name (i.e., the names
	 * of their respective domains).  If equal, then the remote domain
	 * equals the local domain, return 1.
	 */

	if (strcasecmp(ptr1, ptr2) == 0)	/* case insensitive compare */
		return(1);

	return(0);
}
#endif /* NEW_LOGIN */
