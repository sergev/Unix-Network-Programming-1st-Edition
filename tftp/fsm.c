/*
 * Finite state machine routines.
 */

#include	"defs.h"
#include	<signal.h>

#include	"rtt.h"		/* for RTT timing */

#ifdef	CLIENT
int	recv_ACK(), recv_DATA(), recv_RQERR();
#endif

#ifdef	SERVER
int	recv_RRQ(), recv_WRQ(), recv_ACK(), recv_DATA();
#endif

int	fsm_error(), fsm_invalid();

/*
 * Finite state machine table.
 * This is just a 2-d array indexed by the last opcode sent and
 * the opcode just received.  The result is the address of a
 * function to call to process the received opcode.
 */

int	(*fsm_ptr [ OP_MAX + 1 ] [ OP_MAX + 1 ] ) () = {

#ifdef	CLIENT
	fsm_invalid,	/* [sent = 0]        [recv = 0]			*/
	fsm_invalid,	/* [sent = 0]        [recv = OP_RRQ]		*/
	fsm_invalid,	/* [sent = 0]        [recv = OP_WRQ]		*/
	fsm_invalid,	/* [sent = 0]        [recv = OP_DATA]		*/
	fsm_invalid,	/* [sent = 0]        [recv = OP_ACK]		*/
	fsm_invalid,	/* [sent = 0]        [recv = OP_ERROR]		*/

	fsm_invalid,	/* [sent = OP_RRQ]   [recv = 0]			*/
	fsm_invalid,	/* [sent = OP_RRQ]   [recv = OP_RRQ]		*/
	fsm_invalid,	/* [sent = OP_RRQ]   [recv = OP_WRQ]		*/
    recv_DATA,		/* [sent = OP_RRQ]   [recv = OP_DATA]		*/
	fsm_invalid,	/* [sent = OP_RRQ]   [recv = OP_ACK]		*/
    recv_RQERR,		/* [sent = OP_RRQ]   [recv = OP_ERROR]		*/

	fsm_invalid,	/* [sent = OP_WRQ]   [recv = 0]			*/
	fsm_invalid,	/* [sent = OP_WRQ]   [recv = OP_RRQ]		*/
	fsm_invalid,	/* [sent = OP_WRQ]   [recv = OP_WRQ]		*/
	fsm_invalid,	/* [sent = OP_WRQ]   [recv = OP_DATA]		*/
    recv_ACK,		/* [sent = OP_WRQ]   [recv = OP_ACK]		*/
    recv_RQERR,		/* [sent = OP_WRQ]   [recv = OP_ERROR]		*/

	fsm_invalid,	/* [sent = OP_DATA]  [recv = 0]			*/
	fsm_invalid,	/* [sent = OP_DATA]  [recv = OP_RRQ]		*/
	fsm_invalid,	/* [sent = OP_DATA]  [recv = OP_WRQ]		*/
	fsm_invalid,	/* [sent = OP_DATA]  [recv = OP_DATA]		*/
    recv_ACK,		/* [sent = OP_DATA]  [recv = OP_ACK]		*/
	fsm_error,	/* [sent = OP_DATA]  [recv = OP_ERROR]		*/

	fsm_invalid,	/* [sent = OP_ACK]   [recv = 0]			*/
	fsm_invalid,	/* [sent = OP_ACK]   [recv = OP_RRQ]		*/
	fsm_invalid,	/* [sent = OP_ACK]   [recv = OP_WRQ]		*/
    recv_DATA,		/* [sent = OP_ACK]   [recv = OP_DATA]		*/
	fsm_invalid,	/* [sent = OP_ACK]   [recv = OP_ACK]		*/
	fsm_error,	/* [sent = OP_ACK]   [recv = OP_ERROR]		*/

	fsm_invalid,	/* [sent = OP_ERROR] [recv = 0]			*/
	fsm_invalid,	/* [sent = OP_ERROR] [recv = OP_RRQ]		*/
	fsm_invalid,	/* [sent = OP_ERROR] [recv = OP_WRQ]		*/
	fsm_invalid,	/* [sent = OP_ERROR] [recv = OP_DATA]		*/
	fsm_invalid,	/* [sent = OP_ERROR] [recv = OP_ACK]		*/
	fsm_error	/* [sent = OP_ERROR] [recv = OP_ERROR]		*/
#endif	/* CLIENT */

#ifdef	SERVER
	fsm_invalid,	/* [sent = 0]        [recv = 0]			*/
    recv_RRQ,		/* [sent = 0]        [recv = OP_RRQ]		*/
    recv_WRQ,		/* [sent = 0]        [recv = OP_WRQ]		*/
	fsm_invalid,	/* [sent = 0]        [recv = OP_DATA]		*/
	fsm_invalid,	/* [sent = 0]        [recv = OP_ACK]		*/
	fsm_invalid,	/* [sent = 0]        [recv = OP_ERROR]		*/

	fsm_invalid,	/* [sent = OP_RRQ]   [recv = 0]			*/
	fsm_invalid,	/* [sent = OP_RRQ]   [recv = OP_RRQ]		*/
	fsm_invalid,	/* [sent = OP_RRQ]   [recv = OP_WRQ]		*/
	fsm_invalid,	/* [sent = OP_RRQ]   [recv = OP_DATA]		*/
	fsm_invalid,	/* [sent = OP_RRQ]   [recv = OP_ACK]		*/
	fsm_invalid,	/* [sent = OP_RRQ]   [recv = OP_ERROR]		*/

	fsm_invalid,	/* [sent = OP_WRQ]   [recv = 0]			*/
	fsm_invalid,	/* [sent = OP_WRQ]   [recv = OP_RRQ]		*/
	fsm_invalid,	/* [sent = OP_WRQ]   [recv = OP_WRQ]		*/
	fsm_invalid,	/* [sent = OP_WRQ]   [recv = OP_DATA]		*/
	fsm_invalid,	/* [sent = OP_WRQ]   [recv = OP_ACK]		*/
	fsm_invalid,	/* [sent = OP_WRQ]   [recv = OP_ERROR]		*/

	fsm_invalid,	/* [sent = OP_DATA]  [recv = 0]			*/
	fsm_invalid,	/* [sent = OP_DATA]  [recv = OP_RRQ]		*/
	fsm_invalid,	/* [sent = OP_DATA]  [recv = OP_WRQ]		*/
	fsm_invalid,	/* [sent = OP_DATA]  [recv = OP_DATA]		*/
    recv_ACK,		/* [sent = OP_DATA]  [recv = OP_ACK]		*/
	fsm_error,	/* [sent = OP_DATA]  [recv = OP_ERROR]		*/

	fsm_invalid,	/* [sent = OP_ACK]   [recv = 0]			*/
	fsm_invalid,	/* [sent = OP_ACK]   [recv = OP_RRQ]		*/
	fsm_invalid,	/* [sent = OP_ACK]   [recv = OP_WRQ]		*/
    recv_DATA,		/* [sent = OP_ACK]   [recv = OP_DATA]		*/
	fsm_invalid,	/* [sent = OP_ACK]   [recv = OP_ACK]		*/
	fsm_error,	/* [sent = OP_ACK]   [recv = OP_ERROR]		*/

	fsm_invalid,	/* [sent = OP_ERROR] [recv = 0]			*/
	fsm_invalid,	/* [sent = OP_ERROR] [recv = OP_RRQ]		*/
	fsm_invalid,	/* [sent = OP_ERROR] [recv = OP_WRQ]		*/
	fsm_invalid,	/* [sent = OP_ERROR] [recv = OP_DATA]		*/
	fsm_invalid,	/* [sent = OP_ERROR] [recv = OP_ACK]		*/
	fsm_error	/* [sent = OP_ERROR] [recv = OP_ERROR]		*/
#endif	/* SERVER */
};

#ifdef	DATAGRAM
static struct rtt_struct   rttinfo;	/* used by the rtt_XXX() functions */
static int		   rttfirst = 1;

int	tout_flag;	/* set to 1 by SIGALRM handler */
#endif	/* DATAGRAM */

/*
 * Main loop of finite state machine.
 *
 * For the client, we're called after either an RRQ or a WRQ has been
 * sent to the other side.
 *
 * For the server, we're called after either an RRQ or a WRQ has been
 * received from the other side.  In this case, the argument will be a
 * 0 (since nothing has been sent) but the state table above handles
 * this.
 */

int		/* return 0 on normal termination, -1 on timeout */
fsm_loop(opcode)
int	opcode;		/* for client: RRQ or WRQ */
			/* for server: 0 */
{
	register int	nbytes;

	op_sent = opcode;

#ifdef	DATAGRAM

	if (rttfirst) {
		rtt_init(&rttinfo);
		rttfirst = 0;
	}

	rtt_newpack(&rttinfo);		/* initialize for a new packet */

	for ( ; ; ) {
		int	func_timeout();		/* our signal handler */

		signal(SIGALRM, func_timeout);
		tout_flag = 0;
		alarm(rtt_start(&rttinfo));	/* calc timeout & start timer */

		if ( (nbytes = net_recv(recvbuff, MAXBUFF)) < 0) {
			if (tout_flag) {
				/*
				 * The receive timed out.  See if we've tried
				 * enough, and if so, return to caller.
				 */

				if (rtt_timeout(&rttinfo) < 0) {
#ifdef	CLIENT
					printf("Transfer timed out\n");
#endif
					return(-1);
				}

				if (traceflag)
					rtt_debug(&rttinfo);
			} else
				err_dump("net_recv error");

			/*
			 * Retransmit the last packet.
			 */

			net_send(sendbuff, sendlen);
			continue;
		}

		alarm(0);		/* stop signal timer */
		tout_flag = 0;
		rtt_stop(&rttinfo);	/* stop RTT timer, calc new values */

		if (traceflag)
			rtt_debug(&rttinfo);

#else	/* else we have a connection-oriented protocol (makes life easier) */

	for ( ; ; ) {
		if ( (nbytes = net_recv(recvbuff, MAXBUFF)) < 0)
			err_dump("net_recv error");

#endif	/* DATAGRAM */

		if (nbytes < 4)
			err_dump("receive length = %d bytes", nbytes);

		op_recv = ldshort(recvbuff);

		if (op_recv < OP_MIN || op_recv > OP_MAX)
			err_dump("invalid opcode received: %d", op_recv);

		/*
		 * We call the appropriate function, passing the address
		 * of the receive buffer and its length.  These arguments
		 * ignore the received-opcode, which we've already processed.
		 *
		 * We assume the called function will send a response to the
		 * other side.  It is the called function's responsibility to
		 * set op_sent to the op-code that it sends to the other side.
		 */

		if ((*fsm_ptr[op_sent][op_recv])(recvbuff + 2, nbytes - 2) < 0){
			/*
			 * When the called function returns -1, this loop
			 * is done.  Turn off the signal handler for
			 * timeouts and return to the caller.
			 */

			signal(SIGALRM, SIG_DFL);
			return(0);
		}
	}
}

#ifdef	DATAGRAM

/*
 * Signal handler for timeouts.
 * Just set the flag that is looked at above when the net_recv()
 * returns an error (interrupted system call).
 */

int
func_timeout()
{
	tout_flag = 1;		/* set flag for function above */
}

#endif	/* DATAGRAM */

/*
 * Error packet received and we weren't expecting it.
 */

/*ARGSUSED*/
int
fsm_error(ptr, nbytes)
char	*ptr;
int	nbytes;
{
	err_dump("error received: op_sent = %d, op_recv = %d",
					op_sent, op_recv);
}

/*
 * Invalid state transition.  Something is wrong.
 */

/*ARGSUSED*/
int
fsm_invalid(ptr, nbytes)
char	*ptr;
int	nbytes;
{
	err_dump("protocol botch: op_sent = %d, op_recv = %d",
						op_sent, op_recv);
}
