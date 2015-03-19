#define	COUNT	20000

main()
{
	register int	i;

	for (i = 0; i < COUNT; i++) {
		if (getpid() < 0)
			err_sys("getpid error");

		if (getpid() < 0)
			err_sys("getpid error");
	}

	exit(0);
}
