#include	<stdio.h>
#include	"mesg.h"

#define	NUMMESG	10000
#define	MESGLEN	2048

Mesg	mesg;

client(ipcreadfd, ipcwritefd)
int	ipcreadfd;
int	ipcwritefd;
{
	register int	i, n;

	/*
	 * Send a message to the server telling it we're here.
	 */

	mesg.mesg_len = MESGLEN;
	mesg.mesg_type = 1;
	mesg_send(ipcwritefd, &mesg);

	/*
	 * Then read all the messages from the server.
	 */

	for (i = 0; i < NUMMESG; i++) {
		mesg.mesg_type = 0;	/* receive first message on queue */
		if ( (n = mesg_recv(ipcreadfd, &mesg)) <= 0)
			err_sys("client: mesg_recv error");
		if (n != MESGLEN)
			err_sys("client: incorrect length");
		if (mesg.mesg_type != (i + 1))
			err_sys("client: incorrect type");
	}
}

server(ipcreadfd, ipcwritefd)
int	ipcreadfd;
int	ipcwritefd;
{
	register int	i, n;

	/*
	 * Wait for a message from the client telling us it's ready.
	 */

	mesg.mesg_type = 0;	/* receive first message on queue */
	if ( (n = mesg_recv(ipcreadfd, &mesg)) <= 0)
		err_sys("server: mesg_recv error");
	if (n != MESGLEN)
		err_sys("server: incorrect length");
	if (mesg.mesg_type != 1)
		err_sys("server: incorrect type");

	/*
	 * Now send all the messages to the client.
	 */

	mesg.mesg_len = MESGLEN;
	for (i = 0; i < NUMMESG; i++) {
		mesg.mesg_type = i + 1;
		mesg_send(ipcwritefd, &mesg);
	}
}
