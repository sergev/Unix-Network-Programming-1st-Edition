/*
 * Catenate one or more files to standard output.
 * If no files are specified, default to standard input.
 */

#define	BUFFSIZE	4096

main(argc, argv)
int	argc;
char	*argv[];
{
	int		fd, i, n;
	char		buff[BUFFSIZE];
	extern char	*pname;

	pname = argv[0];
	argv++; argc--;

	fd = 0;		/* default to stdin if no arguments */
	i = 0;
	do {
		if (argc > 0 && (fd = my_open(argv[i], 0)) < 0) {
			err_ret("can't open %s", argv[i]);
			continue;
		}

		while ( (n = read(fd, buff, BUFFSIZE)) > 0)
			if (write(1, buff, n) != n)
				err_sys("write error");
		if (n < 0)
			err_sys("read error");

	} while (++i < argc);
	exit(0);
}
