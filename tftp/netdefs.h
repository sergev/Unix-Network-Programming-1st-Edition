/*
 * Network library header file.
 */

#include	"systype.h"
#include	<stdio.h>

#define	MAXHOSTNAMELEN	  64	/* max size of a host name */
#define	MAXLINE		 255	/* line length for error messages */
#define	MAXBUFF		2048	/* max buffer length */

/*
 * Debug macro, based on the traceflag.
 * Note that a daemon typically freopen()s stderr to another file
 * for debugging purposes.
 */

#define	DEBUG(fmt)		if (traceflag) { \
					fprintf(stderr, fmt); \
					fputc('\n', stderr); \
					fflush(stderr); \
				} else ;

#define	DEBUG1(fmt, arg1)	if (traceflag) { \
					fprintf(stderr, fmt, arg1); \
					fputc('\n', stderr); \
					fflush(stderr); \
				} else ;

#define	DEBUG2(fmt, arg1, arg2)	if (traceflag) { \
					fprintf(stderr, fmt, arg1, arg2); \
					fputc('\n', stderr); \
					fflush(stderr); \
				} else ;

#define	DEBUG3(fmt, arg1, arg2, arg3)	if (traceflag) { \
					fprintf(stderr, fmt, arg1, arg2, arg3); \
					fputc('\n', stderr); \
					fflush(stderr); \
				} else ;

#ifdef	BSD
#include	<sys/types.h>
#include	<sys/socket.h>
#include	<netdb.h>
#endif

#ifdef	i386
#include	<sys/types.h>
#include	<sys/socket.h>
#include	<netdb.h>
#endif

#ifdef	XENIX
#include	<sys/types.h>
#include	<sys/socket.h>
			/* there isn't a <netdb.h> for Excelan */

/*
 * Host structure.
 */

struct hostent {
  char	*h_name;	/* official name of host */
  int	h_addrtype;	/* type of address, always AF_INET for now */
  int	h_length;	/* length of address */
  char	*h_addr;	/* address */
};

/*
 * Service structure.
 */

struct servent {
  char	*s_name;	/* official name of service */
  short	s_port;		/* port number for service (in network byte order) */
  char	*s_proto;	/* name of protocol for service */
};

char		*inet_ntoa();
struct hostent	*gethostbyname();
struct servent	*getservbyname();

#endif	/* XENIX */
