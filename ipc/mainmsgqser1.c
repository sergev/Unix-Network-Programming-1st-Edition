#include	<stdio.h>
#include	"mesg.h"
#include	"msgq.h"

Mesg	mesg;

main()
{
	int	id;

	/*
	 * Create the message queue, if required.
	 */

	if ( (id = msgget(MKEY1, PERMS | IPC_CREAT)) < 0)
		err_sys("server: can't get message queue 1");

	server(id);

	exit(0);
}

server(id)
int	id;
{
	int	n, filefd;
	char	errmesg[256], *sys_err_str();

	/*
	 * Read the filename message from the IPC descriptor.
	 */

	mesg.mesg_type = 1L;		/* receive messages of this type */
	if ( (n = mesg_recv(id, &mesg)) <= 0)
		err_sys("server: filename read error");
	mesg.mesg_data[n] = '\0';	/* null terminate filename */

	mesg.mesg_type = 2L;		/* send messages of this type */
	if ( (filefd = open(mesg.mesg_data, 0)) < 0) {
		/*
		 * Error.  Format an error message and send it back
		 * to the client.
		 */

		sprintf(errmesg, ": can't open, %s\n", sys_err_str());
		strcat(mesg.mesg_data, errmesg);
		mesg.mesg_len = strlen(mesg.mesg_data);
		mesg_send(id, &mesg);

	} else {
		/*
		 * Read the data from the file and send a message to
		 * the IPC descriptor.
		 */

		while ( (n = read(filefd, mesg.mesg_data, MAXMESGDATA)) > 0) {
			mesg.mesg_len = n;
			mesg_send(id, &mesg);
		}
		close(filefd);

		if (n < 0)
			err_sys("server: read error");
	}

	/*
	 * Send a message with a length of 0 to signify the end.
	 */

	mesg.mesg_len = 0;
	mesg_send(id, &mesg);
}
