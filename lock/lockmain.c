#define	SEQFILE	"seqno"		/* filename */
#define	MAXBUFF	   100

main()
{
	int	fd, i, n, pid, seqno;
	char	buff[MAXBUFF + 1];

	pid = getpid();
	if ( (fd = open(SEQFILE, 2)) < 0)
		err_sys("can't open %s", SEQFILE);

	for (i = 0; i < 20; i++) {
		my_lock(fd);			/* lock the file */

		lseek(fd, 0L, 0);		/* rewind before read */
		if ( (n = read(fd, buff, MAXBUFF)) <= 0)
			err_sys("read error");
		buff[n] = '\0';		/* null terminate for sscanf */

		if ( (n = sscanf(buff, "%d\n", &seqno)) != 1)
			err_sys("sscanf error");
		printf("pid = %d, seq# = %d\n", pid, seqno);

		seqno++;		/* increment the sequence number */

		sprintf(buff, "%03d\n", seqno);
		n = strlen(buff);
		lseek(fd, 0L, 0);		/* rewind before write */
		if (write(fd, buff, n) != n)
			err_sys("write error");

		my_unlock(fd);			/* unlock the file */
	}
}
