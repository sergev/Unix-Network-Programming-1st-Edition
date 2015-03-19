/*
 *	openfile  <socket-descriptor-number>  <filename>  <mode>
 *
 * Open the specified file using the specified mode.
 * Return the open file descriptor on the specified socket descriptor
 * (which the invoker has to set up before exec'ing us).
 * We exit() with a value of 0 if all is OK, otherwise we pass back the
 * errno value as our exit status.
 */

main(argc, argv)
int	argc;
char	*argv[];
{
	int		fd;
	extern int	errno;
	extern char	*pname;

	pname = argv[0];

	if (argc != 4)
		err_quit("openfile <sockfd#> <filename> <mode>");

	/*
	 * Open the file.
	 */

	if ( (fd = open(argv[2], atoi(argv[3]))) < 0)
		exit( (errno > 0) ? errno : 255 );

	/*
	 * And pass the descriptor back to caller.
	 */

	exit( sendfile(atoi(argv[1]), fd) );
}
