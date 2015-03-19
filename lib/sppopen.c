/*
 * Open a SPP connection.
 */

#include	"netdefs.h"

#include	<netns/ns.h>

/*
 * The following globals are available to the caller, if desired.
 */

struct sockaddr_ns	spp_srv_addr;	/* server's XNS socket addr */

int			/* return socket descriptor if OK, else -1 on error */
spp_open(host, service, port)
char	*host;		/* name of other system to communicate with */
			       /* must be acceptable to ns_addr(3N) */
			       /* <netid><separator><hostid><separator><port> */
char	*service;	/* name of service being requested */
				/* not currently used */
int	port;		/* if < 0, bind a local reserved port (TODO) */
			/* if > 0, it's the port# of server */
{
	int		fd;
	struct ns_addr	ns_addr();	/* BSD library routine */

	if ( (fd = socket(AF_NS, SOCK_STREAM, 0)) < 0) {
		err_ret("spp_open: can't create SPP stream socket");
		return(-1);
	}

	/*
	 * Set up the server's address.
	 */

	bzero((char *) &spp_srv_addr, sizeof(spp_srv_addr));
	spp_srv_addr.sns_family = AF_NS;
	spp_srv_addr.sns_addr   = ns_addr(host);
					/* stores net-ID, host-ID and port */
	if (port > 0)
		spp_srv_addr.sns_addr.x_port = htons(port);
	else if (port < 0)
		err_quit("spp_open: reserved ports not implemeneted yet");

	/*
	 * And connect to the server.
	 */

	if (connect(fd, (struct sockaddr *) &spp_srv_addr,
						sizeof(spp_srv_addr)) < 0) {
		err_ret("spp_open: can't connect to server");
		close(fd);
		return(-1);
	}

	return(fd);	/* all OK */
}
