#define	COUNT		20000
#define	BUFFSIZE	128

char		buff[BUFFSIZE];

main()
{
	register int	i;
	int		pipefd[2];

	if (pipe(pipefd) < 0)
		err_sys("pipe error");

	for (i = 0; i < COUNT; i++) {
		if (write(pipefd[1], buff, BUFFSIZE) < 0)
			err_sys("write error");

		if (read(pipefd[0], buff, BUFFSIZE) != BUFFSIZE)
			err_sys("read error");
	}

	close(pipefd[0]);
	close(pipefd[1]);

	exit(0);
}
