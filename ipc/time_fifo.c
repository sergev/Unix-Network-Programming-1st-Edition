#include	<sys/types.h>
#include	<sys/stat.h>

#define	COUNT		20000
#define	BUFFSIZE	128

#define	FIFO	"/tmp/fifo.temp"

char		buff[BUFFSIZE];

main()
{
	register int	i, fd;

	if (mknod(FIFO, S_IFIFO | 0666, 0) < 0)
		err_sys("mknod error");
	if ( (fd = open(FIFO, 2)) < 0)
		err_sys("open error");

	for (i = 0; i < COUNT; i++) {
		if (write(fd, buff, BUFFSIZE) < 0)
			err_sys("write error");

		if (read(fd, buff, BUFFSIZE) < 0)
			err_sys("read error");
	}

	close(fd);
	if (unlink(FIFO) < 0)
		err_sys("unlink error");

	exit(0);
}
