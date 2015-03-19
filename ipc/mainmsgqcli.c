#include	"msgq.h"

main()
{
	int	readid, writeid;

	/*
	 * Open the message queues.  The server must have
	 * already created them.
	 */

	if ( (writeid = msgget(MKEY1, 0)) < 0)
		err_sys("client: can't msgget message queue 1");
	if ( (readid = msgget(MKEY2, 0)) < 0)
		err_sys("client: can't msgget message queue 2");

	client(readid, writeid);

	/*
	 * Now we can delete the message queues.
	 */

	if (msgctl(readid, IPC_RMID, (struct msqid_ds *) 0) < 0)
		err_sys("client: can't RMID message queue 1");
	if (msgctl(writeid, IPC_RMID, (struct msqid_ds *) 0) < 0)
		err_sys("client: can't RMID message queue 2");

	exit(0);
}
