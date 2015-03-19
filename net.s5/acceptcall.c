/*
 * Accept an incoming connection request.
 *
 * Return the new descriptor that refers to the newly accepted connection,
 * or -1.  The only time we return -1 is if a disconnect arrives before
 * the accept is performed.
 */

#include	<stdio.h>
#include	<tiuser.h>
#include	<fcntl.h>
#include	<stropts.h>

int
accept_call(listenfd, callptr, name, rwflag)
int		listenfd;	/* the descriptor caller used for t_listen() */
struct t_call	*callptr;	/* from t_listen(), passed to t_accept() */
char		*name;		/* name of transport provider */
int		rwflag;		/* if nonzero, push read/write module */
{
	int		newfd;
	extern int	t_errno;

	/*
	 * Open the transport provider to get a new file descriptor.
	 */

	if ( (newfd = t_open(name, O_RDWR, (struct t_info *) 0)) < 0)
		err_sys("t_open error");

	/*
	 * Bind any local address to the new descriptor.  Since this
	 * function is intended to be called by a server after a
	 * connection request has arrived, any local address will suffice.
	 */

	if (t_bind(newfd, (struct t_bind *) 0, (struct t_bind *) 0) < 0)
		err_sys("t_bind error");

	/*
	 * Accept the connection request on the new descriptor.
	 */

	if (t_accept(listenfd, newfd, callptr) < 0) {
		if (t_errno == TLOOK) {
			/*
			 * An asynchronous event has occurred.  We must have
			 * received a disconnect.  Go ahead and call t_rcvdis(),
			 * then close the new file descriptor that we opened
			 * above.
			 */

			if (t_rcvdis(listenfd, (struct t_discon *) 0) < 0)
				err_sys("t_rcvdis error");
			if (t_close(newfd) < 0)
				err_sys("t_close error");
			return(-1);	/* return error to caller */
		}
		err_sys("t_accept error");
	}

	/*
	 * If the caller requests, push the streams module "tirdwr" onto
	 * the new stream, so that the read(2) and write(2) system calls
	 * can be used.  We first have to pop the "timod" module (the
	 * default).
	 */

	if (rwflag) {
		if (ioctl(newfd, I_POP, (char *) 0) < 0)
			err_dump("I_POP of timod failed");

		if (ioctl(newfd, I_PUSH, "tirdwr") < 0)
			err_dump("I_PUSH of tirdwr failed");
	}

	return(newfd);
}
