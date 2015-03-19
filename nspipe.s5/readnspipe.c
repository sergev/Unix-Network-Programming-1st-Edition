/*
 * Create a named stream pipe and read from it.
 */

#include	<sys/types.h>
#include	<stropts.h>

#define	NSPIPENAME	"/tmp/nspipe.serv"
#define	BUFFSIZE	1024

main()
{
	int		fd[2], flags;
	char		cntlbuff[BUFFSIZE], databuff[BUFFSIZE];
	struct strbuf	cntlstr, datastr;

	if (ns_pipe(NSPIPENAME, fd) < 0)
		err_sys("can't create named stream pipe");

	cntlstr.buf    = cntlbuff;
	cntlstr.maxlen = BUFFSIZE;
	cntlstr.len    = 0;

	datastr.buf    = databuff;
	datastr.maxlen = BUFFSIZE;
	datastr.len    = 0;

	flags = 0;

	for ( ; ; ) {
		/*
		 * Since the ns_pipe() function associates the name with
		 * fd[0], we have to read from fd[1] (the other end of
		 * the pipe).
		 */

		if (getmsg(fd[1], &cntlstr, &datastr, &flags) < 0)
			err_sys("getmsg error");
		if (cntlstr.len > 0)
			printf("received %d bytes of control information\n",
					cntlstr.len);
		if (datastr.len > 0)
			printf("data: %.*s\n", datastr.len, datastr.buf);
	}
}
