#include	"mesg.h"

/*
 * Send a message using the System V message queues.
 * The mesg_len, mesg_type and mesg_data fields must be filled
 * in by the caller.
 */

mesg_send(id, mesgptr)
int	id;		/* really an msqid from msgget() */
Mesg	*mesgptr;
{
	/*
	 * Send the message - the type followed by the optional data.
	 */

	if (msgsnd(id, (char *) &(mesgptr->mesg_type),
					mesgptr->mesg_len, 0) != 0)
		err_sys("msgsnd error");
}

/*
 * Receive a message from a System V message queue.
 * The caller must fill in the mesg_type field with the desired type.
 * Return the number of bytes in the data portion of the message.
 * A 0-length data message implies end-of-file.
 */

int
mesg_recv(id, mesgptr)
int	id;		/* really an msqid from msgget() */
Mesg	*mesgptr;
{
	int	n;

	/*
	 * Read the first message on the queue of the specified type.
	 */

	n = msgrcv(id, (char *) &(mesgptr->mesg_type), MAXMESGDATA,
					mesgptr->mesg_type, 0);
	if ( (mesgptr->mesg_len = n) < 0)
		err_dump("msgrcv error");

	return(n);		/* n will be 0 at end of file */
}
