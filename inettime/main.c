/*
 * This program is an example using the DARPA "Daytime Protocol"
 * (see RFC 867 for the details) and the DARPA "Time Protocol"
 * (see RFC 868 for the details).
 *
 * BSD systems provide a server for both of these services,
 * using either UDP/IP or TCP/IP for each service.
 * These services are provided by the "inetd" daemon under 4.3BSD.
 *
 *	inettime  [ -t ]  [ -u ]  hostname ...
 *
 * The -t option says use TCP, and the -u option says use UDP.
 * If neither option is specified, both TCP and UDP are used.
 */

#include	<stdio.h>

#define	MAXHOSTNAMELEN	64

char	hostname[MAXHOSTNAMELEN];
char	*pname;

main(argc, argv)
int   argc;
char  **argv;
{
	int	dotcp, doudp;
	char	*s;

	pname = argv[0];
	dotcp = doudp = 0;

	while (--argc > 0 && (*++argv)[0] == '-')
		for (s = argv[0]+1; *s != '\0'; s++)
			switch (*s) {

			case 't':
				dotcp = 1;
				break;

			case 'u':
				doudp = 1;
				break;

			default:
				err_quit("unknown command line option: %c", *s);
			}

	if (dotcp == 0 && doudp == 0)
		dotcp = doudp = 1;		/* default */

	while (argc-- > 0) {
		strcpy(hostname, *(argv++));

		if (dotcp) {
			tcp_daytime(hostname);
			tcp_time(hostname);
		}

		if (doudp) {
			udp_daytime(hostname);
			udp_time(hostname);
		}
	}

	exit(0);
}
