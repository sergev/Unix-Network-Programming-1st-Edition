/*
 * Establish an IDP socket and optionally call connect() to set up
 * the server's address for future I/O.
 */

#include	"netdefs.h"

#include	<netns/ns.h>

/*
 * The following globals are available to the caller, if desired.
 */

struct sockaddr_ns	idp_srv_addr;	/* server's XNS socket addr */
struct sockaddr_ns	idp_cli_addr;	/* client's XNS socket addr */

int			/* return socket descriptor if OK, else -1 on error */
idp_open(host, service, port, dontconn)
char	*host;		/* name of other system to communicate with */
			       /* must be acceptable to ns_addr(3N) */
			       /* <netid><separator><hostid><separator><port> */
char	*service;	/* name of service being requested */
				/* not currently used */
int	port;		/* if < 0, bind a local reserved port (TODO) */
			/* if > 0, it's the port# of server */
int	dontconn;	/* if == 0, call connect(), else don't */
{
	int		fd;
	struct ns_addr	ns_addr();	/* BSD library routine */

	/*
	 * Create an IDP socket.
	 */

	if ( (fd = socket(AF_NS, SOCK_DGRAM, 0)) < 0) {
		err_ret("idp_open: can't create IDP datagram socket");
		return(-1);
	}

	/*
	 * Bind any local address for us.
	 */

	bzero((char *) &idp_cli_addr, sizeof(idp_cli_addr));
	idp_cli_addr.sns_family = AF_NS;
	if (bind(fd, (struct sockaddr *) &idp_cli_addr,
						sizeof(idp_cli_addr)) < 0) {
		err_ret("idp_open: bind error");
		close(fd);
		return(-1);
	}

	/*
	 * Set up the server's address.
	 */

	bzero((char *) &idp_srv_addr, sizeof(idp_srv_addr));
	idp_srv_addr.sns_family = AF_NS;
	idp_srv_addr.sns_addr   = ns_addr(host);
					/* stores net-ID, host-ID and port */
	if (port > 0)
		idp_srv_addr.sns_addr.x_port = htons(port);
	else if (port < 0)
		err_quit("idp_open: reserved ports not implemeneted yet");

	/*
	 * Call connect, if desired.  This is used by most caller's,
	 * as the peer shouldn't change.
	 * By calling connect, the caller can call send() and recv().
	 */

	if (dontconn == 0) {
		if (connect(fd, (struct sockaddr *) &idp_srv_addr,
						sizeof(idp_srv_addr)) < 0) {
			err_ret("idp_open: connect error");
			return(-1);
		}
	}

	return(fd);
}
