/*
 * Timer routines.
 *
 * These routines are structured so that there are only the following
 * entry points:
 *
 *	void	t_start()	start timer
 *	void	t_stop()	stop timer
 *	double	t_getrtime()	return real (elapsed) time (seconds)
 *	double	t_getutime()	return user time (seconds)
 *	double	t_getstime()	return system time (seconds)
 *
 * Purposely, there are no structures passed back and forth between
 * the caller and these routines, and there are no include files
 * required by the caller.
 */

#include	<stdio.h>
#include	"systype.h"

#ifdef BSD
#include	<sys/time.h>
#include	<sys/resource.h>
#endif

#ifdef SYS5
#include	<sys/types.h>
#include	<sys/times.h>
#include	<sys/param.h>	/* need the definition of HZ */
#define		TICKS	HZ	/* see times(2); usually 60 or 100 */
#endif

#ifdef BSD
static	struct timeval		time_start, time_stop; /* for real time */
static	struct rusage		ru_start, ru_stop;     /* for user & sys time */
#endif

#ifdef SYS5
static	long			time_start, time_stop;
static	struct tms		tms_start, tms_stop;
long				times();
#endif

static	double			start, stop, seconds;

/*
 * Start the timer.
 * We don't return anything to the caller, we just store some
 * information for the stop timer routine to access.
 */

void
t_start()
{

#ifdef BSD
	if (gettimeofday(&time_start, (struct timezone *) 0) < 0)
		err_sys("t_start: gettimeofday() error");
	if (getrusage(RUSAGE_SELF, &ru_start) < 0)
		err_sys("t_start: getrusage() error");
#endif

#ifdef SYS5
	if ( (time_start = times(&tms_start)) == -1)
		err_sys("t_start: times() error");
#endif

}

/*
 * Stop the timer and save the appropriate information.
 */

void
t_stop()
{

#ifdef BSD
	if (getrusage(RUSAGE_SELF, &ru_stop) < 0)
		err_sys("t_stop: getrusage() error");
	if (gettimeofday(&time_stop, (struct timezone *) 0) < 0)
		err_sys("t_stop: gettimeofday() error");
#endif

#ifdef SYS5
	if ( (time_stop = times(&tms_stop)) == -1)
		err_sys("t_stop: times() error");
#endif

}

/*
 * Return the user time in seconds.
 */

double
t_getutime()
{

#ifdef BSD
	start = ((double) ru_start.ru_utime.tv_sec) * 1000000.0
				+ ru_start.ru_utime.tv_usec;
	stop = ((double) ru_stop.ru_utime.tv_sec) * 1000000.0
				+ ru_stop.ru_utime.tv_usec;
	seconds = (stop - start) / 1000000.0;
#endif

#ifdef SYS5
	seconds = (double) (tms_stop.tms_utime - tms_start.tms_utime) /
					(double) TICKS;
#endif

	return(seconds);
}

/*
 * Return the system time in seconds.
 */

double
t_getstime()
{

#ifdef BSD
	start = ((double) ru_start.ru_stime.tv_sec) * 1000000.0
				+ ru_start.ru_stime.tv_usec;
	stop = ((double) ru_stop.ru_stime.tv_sec) * 1000000.0
				+ ru_stop.ru_stime.tv_usec;
	seconds = (stop - start) / 1000000.0;
#endif

#ifdef SYS5
	seconds = (double) (tms_stop.tms_stime - tms_start.tms_stime) /
					(double) TICKS;
#endif

	return(seconds);
}

/*
 * Return the real (elapsed) time in seconds.
 */

double
t_getrtime()
{

#ifdef BSD
	start = ((double) time_start.tv_sec) * 1000000.0
				+ time_start.tv_usec;
	stop = ((double) time_stop.tv_sec) * 1000000.0
				+ time_stop.tv_usec;
	seconds = (stop - start) / 1000000.0;
#endif

#ifdef SYS5
	seconds = (double) (time_stop - time_start) / (double) TICKS;
#endif

	return(seconds);
}
