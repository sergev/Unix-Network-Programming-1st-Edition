/*
 * Print out the packet, if it came from us.  This logic is necessary
 * because ALL readers of the ICMP socket get a copy of ALL ICMP packets
 * which arrive ('tis only fair).  This permits multiple copies of this
 * program to be run without having intermingled output (or statistics!).
 */

#include	"defs.h"

pr_pack(buf, cc, from)
char			*buf;	/* ptr to start of IP header */
int			cc;	/* total size of received packet */
struct sockaddr_in	*from;	/* address of sender */
{
	int			i, iphdrlen, triptime;
	struct ip		*ip;		/* ptr to IP header */
	register struct icmp	*icp;		/* ptr to ICMP header */
	long			*lp;
	struct timeval		tv;
	char			*pr_type();

	from->sin_addr.s_addr = ntohl(from->sin_addr.s_addr);

	if (timing)
		gettimeofday(&tv, (struct timezone *) 0);

	/*
	 * We have to look at the IP header, to get its length.
	 * We also verify that what follows the IP header contains at
	 * least an ICMP header (8 bytes minimum).
	 */

	ip = (struct ip *) buf;
	iphdrlen = ip->ip_hl << 2;	/* convert # 16-bit words to #bytes */
	if (cc < iphdrlen + ICMP_MINLEN) {
		if (verbose)
			printf("packet too short (%d bytes) from %s\n", cc,
				inet_ntoa(ntohl(from->sin_addr.s_addr)));
		return;
	}
	cc -= iphdrlen;

	icp = (struct icmp *)(buf + iphdrlen);
	if (icp->icmp_type != ICMP_ECHOREPLY) {
		/*
		 * The received ICMP packet is not an echo reply.
		 * If the verbose flag was set, we print the first 48 bytes
		 * of the received packet as 12 longs.
		 */

		if (verbose) {
			lp = (long *) buf;	/* to print 12 longs */
			printf("%d bytes from %s: ", cc,
				inet_ntoa(ntohl(from->sin_addr.s_addr)));
			printf("icmp_type=%d (%s)\n",
				icp->icmp_type, pr_type(icp->icmp_type));
			for (i = 0; i < 12; i++)
			    printf("x%2.2x: x%8.8x\n", i*sizeof(long), *lp++);
			printf("icmp_code=%d\n", icp->icmp_code);
		}
		return;
	}

	/*
	 * See if we sent the packet, and if not, just ignore it.
	 */

	if (icp->icmp_id != ident)
		return;

	printf("%d bytes from %s: ", cc,
				     inet_ntoa(ntohl(from->sin_addr.s_addr)));
	printf("icmp_seq=%d. ", icp->icmp_seq);
	if (timing) {
		/*
		 * Calculate the round-trip time, and update the min/avg/max.
		 */

		tvsub(&tv, (struct timeval *) &icp->icmp_data[0]);
		triptime = tv.tv_sec * 1000 + (tv.tv_usec / 1000);
		printf("time=%d. ms", triptime);	/* milliseconds */
		tsum += triptime;
		if (triptime < tmin)
			tmin = triptime;
		if (triptime > tmax)
			tmax = triptime;
	}
	putchar('\n');

	nreceived++;	/* only count echo reply packets that we sent */
}

/*
 * Convert an ICMP "type" field to a printable string.
 * This is called for ICMP packets that are received that are not
 * ICMP_ECHOREPLY packets.
 */

char *
pr_type(t)
register int t;
{
	static char	*ttab[] = {
		"Echo Reply",
		"ICMP 1",
		"ICMP 2",
		"Dest Unreachable",
		"Source Quence",
		"Redirect",
		"ICMP 6",
		"ICMP 7",
		"Echo",
		"ICMP 9",
		"ICMP 10",
		"Time Exceeded",
		"Parameter Problem",
		"Timestamp",
		"Timestamp Reply",
		"Info Request",
		"Info Reply"
	};

	if (t < 0 || t > 16)
		return("OUT-OF-RANGE");

	return(ttab[t]);
}

/*
 * Subtract 2 BSD timeval structs:  out = out - in.
 */

tvsub(out, in)
register struct timeval	*out;	/* return value through pointer */
register struct timeval	*in;
{
	if ( (out->tv_usec -= in->tv_usec) < 0) {	/* subtract microsec */
		out->tv_sec--;
		out->tv_usec += 1000000;
	}
	out->tv_sec -= in->tv_sec;	/* subtract seconds */
}
