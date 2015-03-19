#include	<sys/types.h>
#include	<sys/time.h>

main(argc, argv)
int	argc;
char	*argv[];
{
	long			atol();
	static struct timeval	timeout;

	if (argc != 3)
		err_quit("usage: timer <#seconds> <#microseconds>");
	timeout.tv_sec  = atol(argv[1]);
	timeout.tv_usec = atol(argv[2]);

	if (select(0, (fd_set *) 0, (fd_set *) 0, (fd_set *) 0, &timeout) < 0)
		err_sys("select error");
	exit(0);
}
