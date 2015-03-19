/*
 * Determine which stream modules have been pushed onto a stream.
 */

#include	<stdio.h>
#include	<fcntl.h>
#include	<tiuser.h>
#include	<stropts.h>
#include	<sys/conf.h>

main()
{
	doit("/dev/tcp");
	doit("/dev/udp");
}

doit(name)
char	*name;
{
	int		tfd;
	char		filename[FMNAMESZ + 1];
	struct t_info	info;

	printf("stream = %s\n", name);
	if ( (tfd = t_open(name, O_RDWR, &info)) < 0)
		err_sys("can't open %s", name);

	for ( ; ; ) {
		if (ioctl(tfd, I_LOOK, filename) < 0) {
			err_ret("ioctl I_LOOK error");
			break;
		}

		printf("	module = %s\n", filename);

		if (ioctl(tfd, I_POP, (char *) 0) < 0)
			err_sys("ioctl I_POP error");
	}
	fflush(stdout);
}
