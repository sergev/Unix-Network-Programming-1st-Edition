/*
 * Open a TCP connection.
 */

#include	"netdefs.h"

#include	<netinet/in.h>
#include	<arpa/inet.h>

#ifndef	INADDR_NONE
#define	INADDR_NONE	0xffffffff	/* should be in <netinet/in.h> */
#endif

/*
 * The following globals are available to the caller, if desired.
 */

struct sockaddr_in	tcp_srv_addr;	/* server's Internet socket addr */
struct servent		tcp_serv_info;	/* from getservbyname() */
struct hostent		tcp_host_info;	/* from gethostbyname() */

int			/* return socket descriptor if OK, else -1 on error */
tcp_open(host, service, port)
char	*host;		/* name or dotted-decimal addr of other system */
char	*service;	/* name of service being requested */
			/* can be NULL, iff port > 0 */
int	port;		/* if == 0, nothing special - use port# of service */
			/* if < 0, bind a local reserved port */
			/* if > 0, it's the port# of server (host-byte-order) */
{
	int		fd, resvport;
	unsigned long	inaddr;
	char		*host_err_str();
	struct servent	*sp;
	struct hostent	*hp;

	/*
	 * Initialize the server's Internet address structure.
	 * We'll store the actual 4-byte Internet address and the
	 * 2-byte port# below.
	 */

	bzero((char *) &tcp_srv_addr, sizeof(tcp_srv_addr));
	tcp_srv_addr.sin_family = AF_INET;

	if (service != NULL) {
		if ( (sp = getservbyname(service, "tcp")) == NULL) {
			err_ret("tcp_open: unknown service: %s/tcp", service);
			return(-1);
		}
		tcp_serv_info = *sp;			/* structure copy */
		if (port > 0)
			tcp_srv_addr.sin_port = htons(port);
							/* caller's value */
		else
			tcp_srv_addr.sin_port = sp->s_port;
							/* service's value */
	} else {
		if (port <= 0) {
			err_ret("tcp_open: must specify either service or port");
			return(-1);
		}
		tcp_srv_addr.sin_port = htons(port);
	}

	/*
	 * First try to convert the host name as a dotted-decimal number.
	 * Only if that fails do we call gethostbyname().
	 */

	if ( (inaddr = inet_addr(host)) != INADDR_NONE) {
						/* it's dotted-decimal */
		bcopy((char *) &inaddr, (char *) &tcp_srv_addr.sin_addr,
					sizeof(inaddr));
		tcp_host_info.h_name = NULL;

	} else {
		if ( (hp = gethostbyname(host)) == NULL) {
			err_ret("tcp_open: host name error: %s %s",
						host, host_err_str());
			return(-1);
		}
		tcp_host_info = *hp;	/* found it by name, structure copy */
		bcopy(hp->h_addr, (char *) &tcp_srv_addr.sin_addr,
			hp->h_length);
	}

	if (port >= 0) {
		if ( (fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
			err_ret("tcp_open: can't create TCP socket");
			return(-1);
		}

	} else if (port < 0) {
		resvport = IPPORT_RESERVED - 1;
		if ( (fd = rresvport(&resvport)) < 0) {
			err_ret("tcp_open: can't get a reserved TCP port");
			return(-1);
		}
	}

	/*
	 * Connect to the server.
	 */

	if (connect(fd, (struct sockaddr *) &tcp_srv_addr,
						sizeof(tcp_srv_addr)) < 0) {
		err_ret("tcp_open: can't connect to server");
		close(fd);
		return(-1);
	}

	return(fd);	/* all OK */
}
