/*
 * Print one or more files on a remote line printer.
 *
 *	lpr  [ -h host ] [ -p printer ] [ -P printer ] [ -t ] [ file ... ]
 *
 * If no file arguments are specified, the standard input is read.
 * If any file argument is "-" it also implies the standard input.
 */

#include	"defs.h"

char	*pname;

main(argc, argv)
int	argc;
char	*argv[];
{
	FILE		*fp, *fopen();
	char		*s, *filename;
	int		i;

	pname = argv[0];
	while (--argc > 0 && (*++argv)[0] == '-')
		for (s = argv[0]+1; *s != '\0'; s++)
			switch (*s) {

			case 'P':		/* specify printer name */
			case 'p':		/* specify printer name */
				if (--argc <= 0)
				  err_quit("-%c requires another argument", *s);
				strcpy(printername, *++argv);
				break;

			case 'd':		/* debug */
				debugflag = 1;
				break;

			case 'h':		/* specify host name */
				if (--argc <= 0)
				   err_quit("-h requires another argument");
				strcpy(hostname, *++argv);
				break;

			default:
				fprintf(stderr, "%s: illegal option %c\n",
					        pname, *s);
				break;
			}

	i = 0;
	send_start();
	do {
		if (argc > 0) {
			filename = argv[i];
			if (strcmp(filename, "-") == 0) {
				fp = stdin;
				filename = "-stdin";
			 } else if ( (fp = fopen(argv[i], "r")) == NULL) {
				fprintf(stderr, "%s: can't open %s\n",
								pname, argv[i]);
				continue;
			}
		} else {
			fp = stdin;
			filename = "-stdin";
		}

		send_file(filename, fp);

		fclose(fp);

	} while (++i < argc);
	send_done();

	exit(0);
}
