/*
 * Exec a shell process, dup'ing the specified "fd" as its
 * standard input, standard output and standard error.
 */

#include	<stdio.h>
#include	"systype.h"

exec_shell(fd, argv, envp)
int	fd;		/* communication channel */
char	**argv;
char	**envp;
{
	char	*shell;
	char	*getenv(), *rindex();

	close(0); close(1); close(2);
	if (dup(fd) != 0 || dup(fd) != 1 || dup(fd) != 2)
		err_sys("dup error");
	close(fd);

	/*
	 * We must set up the pathname of the shell to be exec'ed,
	 * and its argv[0].
	 */

	if ( (shell = getenv("SHELL")) == NULL)	/* look at environment */
		shell = "/bin/sh";	/* default */
	if ( (argv[0] = rindex(shell, '/')) != NULL)
		argv[0]++;		/* step past rightmost slash */
	else
		argv[0] = shell;	/* no slashes in pathname */

	execve(shell, argv, envp);
	err_sys("execve error");
		/* NOTREACHED */
}
