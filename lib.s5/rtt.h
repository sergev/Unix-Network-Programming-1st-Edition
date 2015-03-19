/*
 * Definitions for RTT timing.
 */

#include	"systype.h"
#include	<stdio.h>

#ifdef BSD
#include	<sys/time.h>
#include	<sys/resource.h>
#endif

#ifdef SYS5
#include	<sys/times.h>	/* requires <sys/types.h> */
#include	<sys/param.h>	/* need the definition of HZ */
#define		TICKS	HZ	/* see times(2); usually 60 or 100 */
#endif

/*
 * Structure to contain everything needed for RTT timing.
 * One of these required per socket being timed.
 * The caller allocates this structure, then passes its address to
 * all the rtt_XXX() functions.
 */

struct rtt_struct {
  float	rtt_rtt;	/* most recent round-trip time (RTT), seconds */
  float	rtt_srtt;	/* smoothed round-trip time (SRTT), seconds */
  float	rtt_rttdev;	/* smoothed mean deviation, seconds */
  short	rtt_nrexmt;	/* #times retransmitted: 0, 1, 2, ... */
  short	rtt_currto;	/* current retransmit timeout (RTO), seconds */
  short	rtt_nxtrto;	/* retransmit timeout for next packet, if nonzero */

#ifdef BSD
  struct timeval	time_start;	/* for elapsed time */
  struct timeval	time_stop;	/* for elapsed time */
#endif

#ifdef SYS5
  long			time_start;	/* for elapsed time */
  long			time_stop;	/* for elapsed time */
  struct tms		tms_start;	/* arg to times(2), but not used */
  struct tms		tms_stop;	/* arg to times(2), but not used */
#endif

};

#define	RTT_RXTMIN      2	/* min retransmit timeout value, seconds */
#define	RTT_RXTMAX    120	/* max retransmit timeout value, seconds */
#define	RTT_MAXNREXMT 	4	/* max #times to retransmit: must also
				   change exp_backoff[] if this changes */

#ifdef	SYS5
long	times();		/* the system call */
#endif

extern int	rtt_d_flag;	/* can be set nonzero by caller for addl info */
