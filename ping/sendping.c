/*
 * Compose and transmit an ICMP ECHO REQUEST packet.  The IP header
 * will be prepended by the kernel.  The ID field is our UNIX process ID,
 * and the sequence number is an ascending integer.  The first 8 bytes
 * of the data portion are used to hold a BSD UNIX "timeval" struct in host
 * byte-order, to compute the round-trip time of each packet.
 */

#include	"defs.h"

send_ping()
{
	register int		i;
	register struct icmp	*icp;		/* ICMP header */
	register u_char		*uptr;		/* start of user data */

	/*
	 * Fill in the ICMP header.
	 */

	icp = (struct icmp *) sendpack;	/* pointer to ICMP header */
	icp->icmp_type  = ICMP_ECHO;
	icp->icmp_code  = 0;
	icp->icmp_cksum = 0;	     /* init to 0, then call in_cksum() below */
	icp->icmp_id    = ident;     /* our pid, to identify on return */
	icp->icmp_seq   = ntransmitted++;	/* sequence number */

	/*
	 * Add the time stamp of when we sent it.
	 * gettimeofday(2) is a BSD system call that returns the current
	 * local time through its first argument.  The second argument is
	 * for time zone information, which we're not interested in.
	 */

	if (timing)
		gettimeofday((struct timeval *) &sendpack[SIZE_ICMP_HDR],
			     (struct timezone *) 0);

	/*
	 * And fill in the remainder of the packet with the user data.
	 * We just set each byte of udata[i] to i (although this is
	 * not verified when the echoed packet is received back).
	 */

	uptr = &sendpack[SIZE_ICMP_HDR + SIZE_TIME_DATA];
	for (i = SIZE_TIME_DATA; i < datalen; i++)
		*uptr++ = i;

	/*
	 * Compute and store the ICMP checksum (now that we've filled
	 * in the entire ICMP packet).  The checksum includes the ICMP
	 * header, the time stamp, and our user data.
	 */

	icp->icmp_cksum = in_cksum(icp, packsize);

	/*
	 * Now send the datagram.
	 */

	i = sendto(sockfd, sendpack, packsize, 0,
			(struct sockaddr *) &dest, sizeof(dest));
	if (i < 0 || i != packsize)  {
		if (i < 0)
			err_ret("sendto error");
		else
			err_ret("wrote %s %d bytes, return=%d",
						hostname, packsize, i);
	}
}
