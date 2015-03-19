#include	<stdio.h>
#include	"mesg.h"
#include	"msgq.h"

Mesg	mesg;

main()
{
	int	id;

	/*
	 * Open the single message queue.  The server must have
	 * already created it.
	 */

	if ( (id = msgget(MKEY1, 0)) < 0)
		err_sys("client: can't msgget message queue 1");

	client(id);

	/*
	 * Now we can delete the message queue.
	 */

	if (msgctl(id, IPC_RMID, (struct msqid_ds *) 0) < 0)
		err_sys("client: can't RMID message queue 1");

	exit(0);
}

client(id)
int	id;
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
		n--;			/* ignore the newline from fgets() */
	mesg.mesg_data[n] = '\0';	/* overwrite newline at end */
	mesg.mesg_len = n;
	mesg.mesg_type = 1L;		/* send messages of this type */
	mesg_send(id, &mesg);

	/*
	 * Receive the message from the IPC descriptor and write
	 * the data to the standard output.
	 */

	mesg.mesg_type = 2L;	/* receive messages of this type */
	while( (n = mesg_recv(id, &mesg)) > 0)
		if (write(1, mesg.mesg_data, n) != n)
			err_sys("data write error");

	if (n < 0)
		err_sys("data read error");
}
