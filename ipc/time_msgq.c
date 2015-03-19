#include	<sys/types.h>
#include	<sys/ipc.h>
#include	<sys/msg.h>

#define	KEY	((key_t) 54321)

#define	COUNT		20000
#define	BUFFSIZE	  128
#define	PERMS		 0666

main()
{
	register int	i, msqid;
	struct {
	  long	m_type;
	  char  m_text[BUFFSIZE];
	} msgbuff;

	if ( (msqid = msgget(KEY, PERMS | IPC_CREAT)) < 0)
		err_sys("msgget error");
	msgbuff.m_type = 1L;

	for (i = 0; i < COUNT; i++) {
		if (msgsnd(msqid, &msgbuff, BUFFSIZE, 0) < 0)
			err_sys("msgsnd error");

		if (msgrcv(msqid, &msgbuff, BUFFSIZE, 0L, 0) != BUFFSIZE)
			err_sys("msgrcv error");
	}

	if (msgctl(msqid, IPC_RMID, (struct msqid_ds *) 0) < 0)
		err_sys("IPC_RMID error");

	exit(0);
}
