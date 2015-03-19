/*
 * Copyright (c) 1987 Regents of the University of California.
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
"@(#) Copyright (c) 1987 Regents of the University of California.\n\
 All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)ping.c	4.10 (Berkeley) 10/10/88";
#endif /* not lint */

#include	"defs.h"

/*
 *			P I N G . C
 *
 * Using the InterNet Control Message Protocol (ICMP) "ECHO" facility,
 * measure round-trip-delays and packet loss across network paths.
 *
 * Author -
 *	Mike Muuss
 *	U. S. Army Ballistic Research Laboratory
 *	December, 1983
 * Modified at Uc Berkeley
 *
 * Status -
 *	Public Domain.  Distribution Unlimited.
 *
 * Bugs -
 *	More statistics could always be gathered.
 *	This program has to run SUID to ROOT to access the ICMP socket.
 */

char	usage[] = "Usage:  ping [ -drv ] host [ datasize ] [ npackets ]\n";
char	hnamebuf[MAXHOSTNAMELEN];
char	*pname;

main(argc, argv)
int	argc;
char	**argv;
{
	int			sockoptions, on;
	char			*destdotaddr;
	char			*host_err_str();
	struct hostent		*host;
	struct protoent		*proto;

	on = 1;
	pname = argv[0];
	argc--;
	argv++;

	sockoptions = 0;
	while (argc > 0 && *argv[0] == '-') {
		while (*++argv[0]) switch (*argv[0]) {
			case 'd':
				sockoptions |= SO_DEBUG;
				break;
			case 'r':
				sockoptions |= SO_DONTROUTE;
				break;
			case 'v':
				verbose++;
				break;
		}
		argc--, argv++;
	}
	if (argc < 1)
		err_quit(usage);

	/*
	 * Assume the host is specified by numbers (Internet dotted-decimal)
	 * and call inet_addr() to convert it.  If that doesn't work, then
	 * assume its a name and call gethostbyname() to look it up.
	 */

	bzero((char *) &dest, sizeof(dest));
	dest.sin_family = AF_INET;

	if ( (dest.sin_addr.s_addr = inet_addr(argv[0])) != INADDR_NONE) {
		strcpy(hnamebuf, argv[0]);
		hostname = hnamebuf;
		destdotaddr = NULL;
	} else {
		if ( (host = gethostbyname(argv[0])) == NULL) {
			err_quit("host name error: %s %s",
						argv[0], host_err_str());
		}
		dest.sin_family = host->h_addrtype;
		bcopy(host->h_addr, (caddr_t) &dest.sin_addr, host->h_length);
		hostname = host->h_name;
		destdotaddr = inet_ntoa(dest.sin_addr.s_addr);
				/* convert to dotted-decimal notation */
	}

	/*
	 * If the user specifies a size, that is the size of the data area
	 * following the ICMP header that is transmitted.  If the data area
	 * is large enough for a "struct timeval", then enable timing.
	 */

	if (argc >= 2)
		datalen = atoi(argv[1]);
	else
		datalen = DEF_DATALEN;

	packsize = datalen + SIZE_ICMP_HDR;
	if (packsize > MAXPACKET)
		err_quit("packet size too large");
	if (datalen >= SIZE_TIME_DATA)
		timing = 1;

	/*
	 * The user can specify the maximum number of packets to receive.
	 */

	if (argc > 2)
		npackets = atoi(argv[2]);

	/*
	 * Fetch our Unix process ID.  We use that as the "ident" field
	 * in the ICMP header, to identify this process' packets.
	 * This allows multiple copies of ping to be running on a host
	 * at the same time.  This identifier is needed to separate
	 * the received ICMP packets (since all readers of an ICMP
	 * socket get all the received packets).
	 */

	ident = getpid() & 0xffff;

	/*
	 * Create the socket.
	 */

	if ( (proto = getprotobyname("icmp")) == NULL)
		err_quit("unknown protocol: icmp");
	if ( (sockfd = socket(AF_INET, SOCK_RAW, proto->p_proto)) < 0)
		err_sys("can't create raw socket");
	if (sockoptions & SO_DEBUG)
		if (setsockopt(sockfd, SOL_SOCKET, SO_DEBUG, &on,
								sizeof(on)) < 0)
			err_sys("setsockopt SO_DEBUG error");
	if (sockoptions & SO_DONTROUTE)
		if (setsockopt(sockfd, SOL_SOCKET, SO_DONTROUTE, &on,
								sizeof(on)) < 0)
			err_sys("setsockopt SO_DONTROUTE error");

	printf("PING %s", hostname);
	if (destdotaddr)
		printf(" (%s)", destdotaddr);
	printf(": %d data bytes\n", datalen);
	tmin = 99999999;

	setlinebuf(stdout);		/* one line at a time */

	signal(SIGINT, sig_finish);	/* to let user stop program */
	signal(SIGALRM, sig_alarm);	/* invoked every second */

	sig_alarm();	/* start the output going */

	recv_ping();	/* and start the receive */

	/* NOTREACHED */
}
