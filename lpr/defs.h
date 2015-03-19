/*
 * Definitions for line printer client program.
 */

#include	<stdio.h>
#include	<sys/types.h>

#define	MAXFILENAME	  128	/* max filename length */
#define	MAXHOSTNAMELEN	   64	/* max host name length */
#define	MAXLINE		  512	/* max ascii line length */

#define	LPR_SERVICE	"printer"	/* name of the network service */
#define	SEQNO_FILE	"/tmp/seqno"	/* name of the sequence# file */

/*
 * Externals.
 */

extern char	hostname[];	/* name of host providing the service */
extern char	printername[];	/* name of printer to use on hostname */
extern int	debugflag;	/* -d command line options */

/*
 * Debug macro, based on the debug flag (-d command line argument) with
 * two values to print.
 */

#define	DEBUG2(fmt, arg1, arg2)	if (debugflag) { \
					fprintf(stderr, fmt, arg1, arg2); \
					fputc('\n', stderr); \
				} else ;
