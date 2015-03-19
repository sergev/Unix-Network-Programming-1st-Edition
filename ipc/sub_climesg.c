#include	<stdio.h>
#include	"mesg.h"

Mesg	mesg;

client(ipcreadfd, ipcwritefd)
int	ipcreadfd;
int	ipcwritefd;
{
	int	n;

	/*
	 * Read the filename from standard input, write it as
	 * a message to the IPC descriptor.
	 */

	if (fgets(mesg.mesg_data, MAXMESGDATA, stdin) == NULL)
		err_sys("filename read error");

	n = strlen(mesg.mesg_data);
	if (mesg.mesg_data[n-1] == '\n')
		n--;			/* ignore newline from fgets() */
	mesg.mesg_len = n;
	mesg.mesg_type = 1L;
	mesg_send(ipcwritefd, &mesg);

	/*
	 * Receive the message from the IPC descriptor and write
	 * the data to the standard output.
	 */

	while ( (n = mesg_recv(ipcreadfd, &mesg)) > 0)
		if (write(1, mesg.mesg_data, n) != n)
			err_sys("data write error");

	if (n < 0)
		err_sys("data read error");
}
