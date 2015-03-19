/*
 * Copy standard input to standard output.
 */

#define	BUFFSIZE	4096

main()
{
	int		n;
	char		buff[BUFFSIZE];

	while ( (n = read(0, buff, BUFFSIZE)) > 0)
		if (write(1, buff, n) != n)
			err_sys("write error");

	if (n < 0)
		err_sys("read error");

	exit(0);
}
