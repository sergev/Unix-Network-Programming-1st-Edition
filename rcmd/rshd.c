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
static char sccsid[] = "@(#)rshd.c	5.17.1.2 (Berkeley) 2/7/89";
#endif /* not lint */

/*
 * Remote shell server.  We're invoked by the rcmd(3) function.
 */

#include	<sys/param.h>
#include	<sys/ioctl.h>
#include	<sys/socket.h>
#include	<sys/file.h>
#include	<sys/time.h>
#include	<netinet/in.h>
#include	<arpa/inet.h>
#include	<stdio.h>
#include	<varargs.h>
#include	<pwd.h>
#include	<signal.h>
#include	<netdb.h>
#include	<syslog.h>
#include	<errno.h>
extern int	errno;

char	*index();
char	*rindex();
char	*strncat();

int	keepalive = 1;		/* flag for SO_KEEPALIVE socket option */
int	one = 1;		/* used for setsockopt() and ioctl() */

char	env_user[20]  = "USER=";	/* the environment strings we set */
char	env_home[64]  = "HOME=";
char	env_shell[64] = "SHELL=";
char	*env_ptrs[] =
	    {env_home, env_shell, "PATH=/usr/ucb:/bin:/usr/bin:", env_user, 0};
char	**environ;

/*ARGSUSED*/
main(argc, argv)
int	argc;
char	**argv;
{
	int			ch, addrlen;
	struct sockaddr_in	cli_addr;
	struct linger		linger;
	extern int		opterr, optind;		/* in getopt() */
	extern int		_check_rhosts_file;	/* in validuser() */

	openlog("rsh", LOG_PID | LOG_ODELAY, LOG_DAEMON);

	opterr = 0;
	while ( (ch = getopt(argc, argv, "ln")) != EOF)
		switch((char) ch) {
		case 'l':
			_check_rhosts_file = 0;	/* don't check .rhosts file */
			break;

		case 'n':
			keepalive = 0;		/* don't enable SO_KEEPALIVE */
			break;

		case '?':
		default:
			syslog(LOG_ERR, "usage: rshd [-l]");
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
	if (getpeername(0, (struct sockaddr *) &cli_addr, &addrlen) < 0) {
		fprintf(stderr, "%s: ", argv[0]);
		perror("getpeername");
		_exit(1);
	}

	/*
	 * Set the socket options: SO_KEEPALIVE and SO_LINGER.
	 */

	if (keepalive &&
	    setsockopt(0, SOL_SOCKET, SO_KEEPALIVE, (char *) &one,
							      sizeof(one)) < 0)
		syslog(LOG_WARNING, "setsockopt(SO_KEEPALIVE): %m");

	linger.l_onoff = 1;
	linger.l_linger = 60;
	if (setsockopt(0, SOL_SOCKET, SO_LINGER, (char *) &linger,
		       sizeof(linger)) < 0)
		syslog(LOG_WARNING, "setsockopt(SO_LINGER): %m");

	doit(&cli_addr);
		/* doit() never returns */
}

doit(cli_addrp)
struct sockaddr_in	*cli_addrp;	/* client's Internet address */
{
	int			sockfd2, pipefd[2], childpid,
				maxfdp1, cc, oursecport;
	fd_set			ready, readfrom;
	short			clisecport;
	char			*cp, *hostname;
	char			servuname[16], cliuname[16], cmdbuf[NCARGS+1];
	char			remotehost[2 * MAXHOSTNAMELEN + 1];
	char			buf[BUFSIZ], c, sigval;
	struct passwd		*pwd;
	struct hostent		*hp;

	signal(SIGINT, SIG_DFL);
	signal(SIGQUIT, SIG_DFL);
	signal(SIGTERM, SIG_DFL);

#ifdef DEBUG
	{
		int t = open("/dev/tty", 2);
		if (t >= 0) {
			ioctl(t, TIOCNOTTY, (char *) 0);
			close(t);
		}
	}
#endif

	/*
	 * Verify that the client's address is an Internet address.
	 */

	if (cli_addrp->sin_family != AF_INET) {
		syslog(LOG_ERR, "malformed from address\n");
		exit(1);
	}

#ifdef IP_OPTIONS
    {
	u_char		optbuf[BUFSIZ/3], *optptr;
	char		lbuf[BUFSIZ], *lptr;
	int		optsize, ipproto;
	struct protoent	*ip;

	if ( (ip = getprotobyname("ip")) != NULL)
		ipproto = ip->p_proto;
	else
		ipproto = IPPROTO_IP;

	optsize = sizeof(optbuf);
	if (getsockopt(0, ipproto, IP_OPTIONS, (char *) optbuf, &optsize) == 0
	    && optsize != 0) {
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
#endif

	/*
	 * Verify that the client's address was bound to a reserved port.
	 */

	cli_addrp->sin_port = ntohs((u_short) cli_addrp->sin_port);
				/* need host byte ordered port# to compare */
	if (cli_addrp->sin_port >= IPPORT_RESERVED  ||
	    cli_addrp->sin_port <  IPPORT_RESERVED/2) {
		syslog(LOG_NOTICE, "Connection from %s on illegal port",
					inet_ntoa(cli_addrp->sin_addr));
		exit(1);
	}

	/*
	 * Read the ASCII string specifying the secondary port# from
	 * the socket.  We set a timer of 60 seconds to do this read,
	 * else we assume something is wrong.  If the client doesn't want
	 * the secondary port, they just send the terminating null byte.
	 */

	alarm(60);
	clisecport = 0;
	for ( ; ; ) {
		if ( (cc = read(0, &c, 1)) != 1) {
			if (cc < 0)
				syslog(LOG_NOTICE, "read: %m");
			shutdown(0, 2);
			exit(1);
		}
		if (c == 0)		/* null byte terminates the string */
			break;
		clisecport = (clisecport * 10) + (c - '0');
	}
	alarm(0);

	if (clisecport != 0) {
		/*
		 * If the secondary port# is nonzero, then we have to
		 * connect to that port (which the client has already
		 * created and is listening on).  The secondary port#
		 * that the client tells us to connect to has to also be
		 * a reserved port#.  Also, our end of this secondary
		 * connection has to also have a reserved TCP port bound
		 * to it, plus.
		 */

		if (clisecport >= IPPORT_RESERVED) {
			syslog(LOG_ERR, "2nd port not reserved\n");
			exit(1);
		}

		oursecport = IPPORT_RESERVED - 1; /* starting port# to try */
		if ( (sockfd2 = rresvport(&oursecport)) < 0) {
			syslog(LOG_ERR, "can't get stderr port: %m");
			exit(1);
		}

		/*
		 * Use the cli_addr structure that we already have.
		 * The 32-bit Internet address is obviously that of the
		 * client's, just change the port# to the one specified
		 * by the client as the secondary port.
		 */

		cli_addrp->sin_port = htons((u_short) clisecport);
		if (connect(sockfd2, (struct sockaddr *) cli_addrp,
			    sizeof(*cli_addrp)) < 0) {
			syslog(LOG_INFO, "connect second port: %m");
			exit(1);
		}
	}

	/*
	 * Get the "name" of the client from its Internet address.
	 * This is used for the authentication below.
	 */

	hp = gethostbyaddr((char *) &cli_addrp->sin_addr,
				sizeof(struct in_addr), cli_addrp->sin_family);
	if (hp) {
		/*
		 * If the name returned by gethostbyaddr() is in our domain,
		 * attempt to verify that we haven't been fooled by someone
		 * in a remote net.  Look up the name and check that this
		 * address corresponds to the name.
		 */

		if (local_domain(hp->h_name)) {
			strncpy(remotehost, hp->h_name, sizeof(remotehost) - 1);
			remotehost[sizeof(remotehost) - 1] = 0;
			if ( (hp = gethostbyname(remotehost)) == NULL) {
				syslog(LOG_INFO,
				    "Couldn't look up address for %s",
				    				remotehost);
				my_error("Couldn't look up addr for your host");
				exit(1);
			}
			for ( ; ; hp->h_addr_list++) {
				if (bcmp(hp->h_addr_list[0],
			    		      (caddr_t) &cli_addrp->sin_addr,
			    		      sizeof(cli_addrp->sin_addr)) == 0)
					break;	/* equal, OK */

				if (hp->h_addr_list[0] == NULL) {
					syslog(LOG_NOTICE,
				  	  "Host addr %s not listed for host %s",
				    		inet_ntoa(cli_addrp->sin_addr),
				    		hp->h_name);
					my_error("Host address mismatch");
					exit(1);
				}
			}
		}
		hostname = hp->h_name;
	} else
		hostname = inet_ntoa(cli_addrp->sin_addr);

	/*
	 * Read three strings from the client.
	 */

	getstr(cliuname, sizeof(cliuname), "cliuname");
	getstr(servuname, sizeof(servuname), "servuname");
	getstr(cmdbuf, sizeof(cmdbuf), "command");

	/*
	 * Look up servuname in the password file.  The servuname has
	 * to be a valid account on this system.
	 */

	setpwent();
	if ( (pwd = getpwnam(servuname)) == (struct passwd *) NULL) {
		my_error("Login incorrect.\n");
		exit(1);
	}
	endpwent();

	/*
	 * We'll execute the client's command in the home directory
	 * of servuname.
	 */

	if (chdir(pwd->pw_dir) < 0) {
		chdir("/");
#ifdef notdef
		my_error("No remote directory.\n");
		exit(1);
#endif
	}

	if (pwd->pw_passwd != NULL  &&  *pwd->pw_passwd != '\0'  &&
	    ruserok(hostname, pwd->pw_uid == 0, cliuname, servuname) < 0) {
		my_error("Permission denied.\n");
		exit(1);
	}

	/*
	 * If the servuname isn't root, then check if logins are disabled.
	 */

	if (pwd->pw_uid != 0  &&  access("/etc/nologin", F_OK) == 0) {
		my_error("Logins currently disabled.\n");
		exit(1);
	}

	/*
	 * Now write the null byte back to the client telling it
	 * that everything is OK.
	 * Note that this means that any error messages that we generate
	 * from now on (such as the perror() if the execl() fails), won't
	 * be seen by the rcmd() function, but will be seen by the
	 * application that called rcmd() when it reads from the socket.
	 */

	if (write(2, "", 1) != 1)
		exit(1);

	if (clisecport) {
		/*
		 * We need a secondary channel.  Here's where we create
		 * the control process that'll handle this secondary
		 * channel.
		 * First create a pipe to use for communication between
		 * the parent and child, then fork.
		 */

		if (pipe(pipefd) < 0) {
			my_error("Can't make pipe.\n");
			exit(1);
		}

		if ( (childpid = fork()) == -1) {
			my_error("Try again.\n");
			exit(1);
		}

		if (pipefd[0] > sockfd2)	/* set max fd + 1 for select */
			maxfdp1 = pipefd[0];
		else
			maxfdp1 = sockfd2;
		maxfdp1++;

		if (childpid != 0) {
			/*
			 * Parent process == control process.
			 * We: (1) read from the pipe and write to sockfd2;
			 *     (2) read from sockfd2 and send corresponding
			 *	   signal.
			 */

			close(0);	/* child handles the original socket */
			close(1);	/* (0, 1, and 2 were from inetd) */
			close(2);
			close(pipefd[1]);	/* close write end of pipe */

			FD_ZERO(&readfrom);
			FD_SET(sockfd2, &readfrom);
			FD_SET(pipefd[0], &readfrom);

			ioctl(pipefd[0], FIONBIO, (char *) &one);
					/* should set sockfd2 nbio! */
			do {
				ready = readfrom;
				if (select(maxfdp1, &ready, (fd_set *) 0,
					(fd_set *) 0, (struct timeval *) 0) < 0)
					      /* wait until something to read */
					break;

				if (FD_ISSET(sockfd2, &ready)) {
					if (read(sockfd2, &sigval, 1) <= 0)
						FD_CLR(sockfd2, &readfrom);
					else
						killpg(childpid, sigval);
				}

				if (FD_ISSET(pipefd[0], &ready)) {
					errno = 0;
					cc = read(pipefd[0], buf, sizeof(buf));
					if (cc <= 0) {
						shutdown(sockfd2, 2);
						FD_CLR(pipefd[0], &readfrom);
					} else
						write(sockfd2, buf, cc);
				}
			} while (FD_ISSET(sockfd2, &readfrom) ||
				 FD_ISSET(pipefd[0], &readfrom));
			/*
			 * The pipe will generate an EOF when the shell
			 * terminates.  The socket will terminate when the
			 * client process terminates.
			 */

			exit(0);
		}

		/*
		 * Child process.  Become a process group leader, so that
		 * the control process above can send signals to all the
		 * processes we may be the parent of.  The process group ID
		 * (the getpid() value below) equals the childpid value from
		 * the fork above.
		 */

		setpgrp(0, getpid());
		close(sockfd2);		/* control process handles this fd */
		close(pipefd[0]);	/* close read end of pipe */
		dup2(pipefd[1], 2);	/* stderr of shell has to go through
					   pipe to control process */
		close(pipefd[1]);
	}

	if (*pwd->pw_shell == '\0')
		pwd->pw_shell = "/bin/sh";

	/*
	 * Set the gid, then uid to become the user specified by "servuname".
	 */

	setgid((gid_t) pwd->pw_gid);
	initgroups(pwd->pw_name, pwd->pw_gid);	/* BSD groups */
	setuid((uid_t) pwd->pw_uid);

	/*
	 * Set up an initial environment for the shell that we exec().
	 */

	environ = env_ptrs;
	strncat(env_home,  pwd->pw_dir,   sizeof(env_home)-6);
	strncat(env_shell, pwd->pw_shell, sizeof(env_shell)-7);
	strncat(env_user,  pwd->pw_name,  sizeof(env_user)-6);

	if ( (cp = rindex(pwd->pw_shell, '/')) != NULL)
		cp++;			/* step past first slash */
	else
		cp = pwd->pw_shell;	/* no slash in shell string */

	execl(pwd->pw_shell, cp, "-c", cmdbuf, (char *) 0);

	perror(pwd->pw_shell);		/* error from execl() */
	exit(1);
}

/*
 * Read a string from the socket.  Make sure it fits, else fatal error.
 */

getstr(buf, cnt, errmesg)
char	*buf;
int	cnt;		/* sizeof() the char array */
char	*errmesg;	/* in case error message required */
{
	char	c;

	do {
		if (read(0, &c, 1) != 1)
			exit(1);	/* error or EOF */
		*buf++ = c;
		if (--cnt == 0) {
			my_error("%s too long.\n", errmesg);
			exit(1);
		}
	} while (c != 0);	/* null byte terminates the string */
}

/*
 * Send an error message back to the rcmd() client.
 * The first byte we send must be binary 1, followed by the ASCII
 * error message, followed by a newline.
 */

my_error(va_alist)
va_dcl
{
	va_list		args;
	char		*fmt, buff[BUFSIZ];

	va_start(args);
	fmt = va_arg(args, char *);
	buff[0] = 1;
	vsprintf(buff + 1, fmt, args);
	va_end(args);

	write(2, buff, strlen(buff));	/* fd 2 = socket, from inetd */
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
