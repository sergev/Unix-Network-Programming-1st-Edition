/*
 * This routine causes another PING to be transmitted, and then
 * schedules another SIGALRM for 1 second from now.
 *
 * 	Our sense of time will slowly skew (i.e., packets will not be launched
 * 	exactly at 1-second intervals).  This does not affect the quality
 *	of the delay and loss statistics.
 */

#include	"defs.h"

sig_alarm()
{
	int	waittime;

	send_ping();		/* first send another packet */

	if (npackets == 0 || ntransmitted < npackets)
		/*
		 * If we're not sending a fixed number of packets,
		 * or if we are sending a fixed number but we've still
		 * got more to send, schedule another signal for 1 second
		 * from now.
		 */

		alarm(1);

	else {
		/*
		 * We've sent the specified number of packets.
		 * But, we can't just terminate, as there is at least one
		 * packet still to be received (the one we sent at the
		 * beginning of this function).
		 * If we've received at least one packet already, then
		 * wait for 2 times the largest round-trip time we've seen
		 * so far.  Otherwise we haven't received anything yet from
		 * the host we're pinging, so just wait 10 seconds.
		 */

		if (nreceived) {
			waittime = 2 * tmax / 1000;	/* tmax is millisec */
			if (waittime == 0)
				waittime = 1;
		} else
			waittime = MAXWAIT;

		signal(SIGALRM, sig_finish);	/* change the signal handler */
		alarm(waittime);		/* schedule the signal */
	}
	return;
}
