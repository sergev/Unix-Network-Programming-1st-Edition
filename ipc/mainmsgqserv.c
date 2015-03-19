#include	"msgq.h"

main()
{
	int	readid, writeid;

	/*
	 * Create the message queues, if required.
	 */

	if ( (readid = msgget(MKEY1, PERMS | IPC_CREAT)) < 0)
		err_sys("server: can't get message queue 1");
	if ( (writeid = msgget(MKEY2, PERMS | IPC_CREAT)) < 0)
		err_sys("server: can't get message queue 2");

	server(readid, writeid);

	exit(0);
}
