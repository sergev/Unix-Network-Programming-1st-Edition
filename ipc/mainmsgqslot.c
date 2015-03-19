#include	<sys/types.h>
#include	<sys/ipc.h>
#include	<sys/msg.h>

#define	KEY	((key_t) 98765L)
#define	PERMS	0666

main()
{
	int	i, msqid;

	for (i = 0; i < 10; i++) {
		if ( (msqid = msgget(KEY, PERMS | IPC_CREAT)) < 0)
			err_sys("can't create message queue");

		printf("msqid = %d\n", msqid);

		if (msgctl(msqid, IPC_RMID, (struct msqid_ds *) 0) < 0)
			err_sys("can't remove message queue");
	}
}
