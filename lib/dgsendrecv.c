/*
 * Send a datagram to a server, and read a response.
 * Establish a timer and resend as necessary.
 * This function is intended for those applications that send a datagram
 * and expect a response.
 * Returns actual size of received datagram, or -1 if error or no response.
 */

#include	<sys/types.h>
#include	<sys/socket.h>
#include	<signal.h>
extern int	errno;

#include	"rtt.h"		/* our header for RTT calculations */

static struct rtt_struct   rttinfo;	/* used by rtt_XXX() functions */
static int		   rttfirst = 1;
static int		   tout_flag;	/* used in this file only */

int
dgsendrecv(fd, outbuff, outbytes, inbuff, inbytes, destaddr, destlen)
int		fd;		/* datagram socket */
char		*outbuff;	/* pointer to buffer to send */
int		outbytes;	/* #bytes to send */
char		*inbuff;	/* pointer to buffer to receive into */
int		inbytes;	/* max #bytes to receive */
struct sockaddr *destaddr;	/* destination address */
				/* can be 0, if datagram socket is connect'ed */
int		destlen;	/* sizeof(destaddr) */
{
	int	n;
	int	to_alarm();		/* our alarm() signal handler */

	if (rttfirst == 1) {
		rtt_init(&rttinfo);	/* initialize first time we're called */
		rttfirst = 0;
	}

	rtt_newpack(&rttinfo);		/* initialize for new packet */
rexmit:
	/*
	 * Send the datagram.
	 */

	if (sendto(fd, outbuff, outbytes, 0, destaddr, destlen) != outbytes) {
		err_ret("dgsendrecv: sendto error on socket");
		return(-1);
	}

	signal(SIGALRM, to_alarm);
	tout_flag = 0;			/* for signal handler */
	alarm(rtt_start(&rttinfo));	/* calc timeout value & start timer */

	n = recvfrom(fd, inbuff, inbytes, 0,
			(struct sockaddr *) 0, (int *) 0);
	if (n < 0) {
		if (tout_flag) {
			/*
			 * The recvfrom() above timed out.
			 * See if we've retransmitted enough, and
			 * if so quit, otherwise try again.
			 */

			if (rtt_timeout(&rttinfo) < 0) {
#ifdef	DEBUG
				err_ret("dgsendrecv: no response from server");
#endif
				rttfirst = 1;	/* reinit if called again */
				return(-1);
					/* errno will be EINTR */
			}

			/*
			 * We have to send the datagram again.
			 */

			errno = 0;		/* clear the error flag */
#ifdef	DEBUG
			err_ret("dgsendrecv: timeout, retransmitting");
			rtt_d_flag = 1;
			rtt_debug(&rttinfo);
#endif
			goto rexmit;
		} else {
			err_ret("dgsendrecv: recvfrom error");
			return(-1);
		}
	}

	alarm(0);		/* stop signal timer */
	rtt_stop(&rttinfo);	/* stop RTT timer, calc & store new values */

#ifdef	DEBUG
	rtt_debug(&rttinfo);
#endif

	return(n);		/* return size of received datagram */
}

/*
 * Signal handler for timeouts (SIGALRM).
 * This function is called when the alarm() value that was set counts
 * down to zero.  This indicates that we haven't received a response
 * from the server to the last datagram we sent.
 * All we do is set a flag and return from the signal handler.
 * The occurrence of the signal interrupts the recvfrom() system call
 * (errno = EINTR) above, and we then check the tout_flag flag.
 */

to_alarm()
{
	tout_flag = 1;		/* set flag for function above */
}
