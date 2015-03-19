/*
 * Recording process, first try: use stream pipes.
 */

main(argc, argv, envp)
int	argc;
char	**argv;
char	**envp;
{
	int	fd[2], childpid;

	if (!isatty(0) || !isatty(1))
		err_quit("stdin and stdout must be a terminal");

	if (s_pipe(fd) < 0)
		err_sys("can't create stream pipe");

	if ( (childpid = fork()) < 0)
		err_sys("can't fork");
	else if (childpid == 0) {	/* child process */
		close(fd[0]);
		exec_shell(fd[1], argv, envp);
			/* NOTREACHED */
	}

	close(fd[1]);			/* parent process */
	pass_all(fd[0], childpid);
	exit(0);
}
