/*
 * tftp - Trivial File Transfer Program.  Client side.
 *
 * See RFC 783 for details.  Also see the "Requirements for Internet Hosts"
 * RFC for additional explanations and clarifications.
 */

#include	"defs.h"
#include	<signal.h>

main(argc, argv)
int   argc;
char  **argv;
{
	register int	i;
	register char	*s;
	register FILE	*fp;

	pname = argv[0];

	while (--argc > 0 && (*++argv)[0] == '-')
		for (s = argv[0]+1; *s != '\0'; s++)
			switch (*s) {

			case 'h':		/* specify host name */
				if (--argc <= 0)
				   err_quit("-h requires another argument");
				strcpy(hostname, *++argv);
				break;

			case 't':
				traceflag = 1;
				break;

			case 'v':
				verboseflag = 1;
				break;

			default:
				err_quit("unknown command line option: %c", *s);
			}

	/*
	 * For each filename argument, execute the tftp commands in
	 * that file.  If no filename arguments were specified on the
	 * command line, we process the standard input by default.
	 */

	i = 0;
	fp = stdin;
	do {
		if (argc > 0 && (fp = fopen(argv[i], "r")) == NULL) {
			err_sys("%s: can't open %s for reading", argv[i]);
		}

		mainloop(fp);		/* process a given file */

	} while (++i < argc);

	exit(0);
}

mainloop(fp)
FILE	*fp;
{
	int		sig_intr();

	if (signal(SIGINT, SIG_IGN) != SIG_IGN)
		signal(SIGINT, sig_intr);

	/*
	 * Main loop.  Read a command and execute it.
	 * This loop is terminated by a "quit" command, or an
	 * end-of-file on the command stream.
	 */

	if (setjmp(jmp_mainloop) < 0) {
		err_ret("Timeout");
	}

	if (interactive)
		printf("%s", prompt);

	while (getline(fp)) {
		if (gettoken(command) != NULL)
			docmd(command);

		if (interactive)
			printf("%s", prompt);
	}
}

/*
 * INTR signal handler.  Just return to the main loop above.
 * In case we were waiting for a read to complete, turn off any possible
 * alarm clock interrupts.
 *
 * Note that with TFTP, if the client aborts a file transfer (such as with
 * the interrupt signal), the server is not notified.  The protocol counts
 * on the server eventually timing out and exiting.
 */

int
sig_intr()
{
	signal(SIGALRM, SIG_IGN);	/* first ignore the signal */
	alarm(0);			/* then assure alarm is off */

	longjmp(jmp_mainloop, 1);
	/* NOTREACHED */
}
