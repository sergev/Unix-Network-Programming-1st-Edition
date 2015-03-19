/*
 * Recording process, second try: now use pseudo-terminals.
 */

main(argc, argv, envp)
int	argc;
char	**argv;
char	**envp;
{
	int	master_fd, slave_fd, childpid;

	if (!isatty(0) || !isatty(1))
		err_quit("stdin and stdout must be a terminal");

	master_fd = pty_master();
	if (master_fd < 0)
		err_sys("can't open master pty");

	if ( (childpid = fork()) < 0)
		err_sys("can't fork");
	else if (childpid == 0) {	/* child process */
		slave_fd = pty_slave(master_fd);
		if (slave_fd < 0)
			err_sys("can't open pty slave");
		close(master_fd);

		exec_shell(slave_fd, argv, envp);
			/* NOTREACHED */
	}

	pass_all(master_fd, childpid);
	exit(0);
}
