/*
 * tftp - Trivial File Transfer Program.  Server side.
 *
 *	-i		says we were *not* started by inted.
 *	-p port#	specifies a different port# to listen on.
 *	-t		turns on the traceflag - writes to a file.
 */

#include	"defs.h"

main(argc, argv)
int   argc;
char  **argv;
{
	int		childpid;
	register char	*s;

	err_init("rich's tftpd");

	while (--argc > 0 && (*++argv)[0] == '-')
		for (s = argv[0]+1; *s != '\0'; s++)
			switch (*s) {
			case 'i':
				inetdflag = 0;	/* turns OFF the flag */
						/* (it defaults to 1) */
				break;

			case 'p':		/* specify server's port# */
				if (--argc <= 0)
				   err_quit("-p requires another argument");
				port = atoi(*++argv);
				break;

			case 't':
				traceflag = 1;
				break;

			default:
				err_quit("unknown command line option: %c", *s);
			}

	if (inetdflag == 0) {
		/*
		 * Start us up as a daemon process (in the background).
		 * Also initialize the network connection - create the socket
		 * and bind our well-known address to it.
		 */

		daemon_start(1);

		net_init(TFTP_SERVICE, port);
	}

	/*
	 * If the traceflag is set, open a log file to write to.
	 * This is used by the DEBUG macros.  Note that all the
	 * err_XXX() functions still get handled by syslog(3).
	 */

	if (traceflag) {
		if (freopen(DAEMONLOG, "a", stderr) == NULL)
			err_sys("can't open %s for writing", DAEMONLOG);
		DEBUG2("pid = %d, inetdflag = %d", getpid(), inetdflag);
	}

	/*
	 * Concurrent server loop.
	 * The child created by net_open() handles the client's request.
	 * The parent waits for another request.  In the inetd case,
	 * the parent from net_open() never returns.
	 */

	for ( ; ; ) {
		if ( (childpid = net_open(inetdflag)) == 0) {
			fsm_loop(0);	/* child processes client's request */
			net_close();	/* then we're done */
			exit(0);
		}

		/* parent waits for another client's request */
	}
	/* NOTREACHED */
}
