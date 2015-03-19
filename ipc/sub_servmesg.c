#include	<stdio.h>
#include	"mesg.h"

extern int	errno;

Mesg	mesg;

server(ipcreadfd, ipcwritefd)
int	ipcreadfd;
int	ipcwritefd;
{
	int	n, filefd;
	char	errmesg[256], *sys_err_str();

	/*
	 * Read the filename message from the IPC descriptor.
	 */

	mesg.mesg_type = 1L;
	if ( (n = mesg_recv(ipcreadfd, &mesg)) <= 0)
		err_sys("server: filename read error");
	mesg.mesg_data[n] = '\0';	/* null terminate filename */

	if ( (filefd = open(mesg.mesg_data, 0)) < 0) {
		/*
		 * Error.  Format an error message and send it back
		 * to the client.
		 */

		sprintf(errmesg, ": can't open, %s\n", sys_err_str());
		strcat(mesg.mesg_data, errmesg);
		mesg.mesg_len = strlen(mesg.mesg_data);
		mesg_send(ipcwritefd, &mesg);

	} else {
		/*
		 * Read the data from the file and send a message to
		 * the IPC descriptor.
		 */

		while ( (n = read(filefd, mesg.mesg_data, MAXMESGDATA)) > 0) {
			mesg.mesg_len = n;
			mesg_send(ipcwritefd, &mesg);
		}
		close(filefd);

		if (n < 0)
			err_sys("server: read error");
	}

	/*
	 * Send a message with a length of 0 to signify the end.
	 */

	mesg.mesg_len = 0;
	mesg_send(ipcwritefd, &mesg);
}
