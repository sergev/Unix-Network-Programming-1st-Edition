/*
 * TFTP network handling for TCP/IP connection.
 */

#include	"netdefs.h"

#include	<netinet/in.h>
#include	<arpa/inet.h>
#include	<signal.h>
#include	<errno.h>
extern int	errno;

#ifndef	CLIENT
#ifndef	SERVER
either CLIENT or SERVER must be defined
#endif
#endif

int	sockfd = -1;				/* fd for socket of server */
char	openhost[MAXHOSTNAMELEN] = { 0 };	/* remember host's name */

extern int			traceflag;	/* TFTP variable */

extern struct sockaddr_in	tcp_srv_addr;	/* set by tcp_open() */
extern struct servent		tcp_serv_info;	/* set by tcp_open() */

#ifdef	CLIENT

/*
 * Open the network connection.  Client version.
 */

int
net_open(host, service, port)
char	*host;		/* name of other system to communicate with */
char	*service;	/* name of service being requested */
int	port;		/* if > 0, use as port#, else use value for service */
{
	if ( (sockfd = tcp_open(host, service, port)) < 0)
		return(-1);

	DEBUG2("net_open: host %s, port# %d",
			inet_ntoa(tcp_srv_addr.sin_addr),
			ntohs(tcp_srv_addr.sin_port));

	strcpy(openhost, host);		/* save the host's name */

	return(0);
}

#endif	/* CLIENT */

/*
 * Close the network connection.  Used by client and server.
 */

net_close()
{
	DEBUG2("net_close: host = %s, fd = %d", openhost, sockfd);

	close(sockfd);

	sockfd = -1;
}

/*
 * Send a record to the other end.  Used by client and server.
 * With a stream socket we have to preface each record with its length,
 * since TFTP doesn't have a record length as part of each record.
 * We encode the length as a 2-byte integer in network byte order.
 */

net_send(buff, len)
char	*buff;
int	len;
{
	register int	rc;
	short		templen;

	DEBUG1("net_send: sent %d bytes", len);

	templen = htons(len);
	rc = writen(sockfd, (char *) &templen, sizeof(short));
	if (rc != sizeof(short))
		err_dump("writen error of length prefix");

	rc = writen(sockfd, buff, len);
	if (rc != len)
		err_dump("writen error");
}

/*
 * Receive a record from the other end.  Used by client and server.
 */

int				/* return #bytes in packet, or -1 on EINTR */
net_recv(buff, maxlen)
char	*buff;
int	maxlen;
{
	register int	nbytes;
	short		templen;	/* value-result parameter */

again1:
	if ( (nbytes = readn(sockfd, (char *) &templen, sizeof(short))) < 0) {
		if (errno == EINTR) {
			errno = 0;		/* assume SIGCLD */
			goto again1;
		}
		err_dump("readn error for length prefix");
	}
	if (nbytes != sizeof(short))
		err_dump("error in readn of length prefix");

	templen = ntohs(templen);		/* #bytes that follow */
	if (templen > maxlen)
		err_dump("record length too large");

again2:
	if ( (nbytes = readn(sockfd, buff, templen)) < 0) {
		if (errno == EINTR) {
			errno = 0;		/* assume SIGCLD */
			goto again2;
		}
		err_dump("readn error");
	}
	if (nbytes != templen)
		err_dump("error in readn");

	DEBUG1("net_recv: got %d bytes", nbytes);

	return(nbytes);		/* return the actual length of the message */
}

#ifdef	SERVER

struct sockaddr_in		tcp_cli_addr;	/* set by accept() */

/*
 * Initialize the network connection for the server, when it has *not*
 * been invoked by inetd.
 */

net_init(service, port)
char	*service;	/* the name of the service we provide */
int	port;		/* if nonzero, this is the port to listen on;
			   overrides the standard port for the service */
{
	struct servent	*sp;

	/*
	 * We weren't started by a master daemon.
	 * We have to create a socket ourselves and bind our well-known
	 * address to it.
	 */

	bzero((char *) &tcp_srv_addr, sizeof(tcp_srv_addr));
	tcp_srv_addr.sin_family      = AF_INET;
	tcp_srv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (service != NULL) {
		if ( (sp = getservbyname(service, "tcp")) == NULL)
			err_dump("net_init: unknown service: %s/tcp", service);
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
	 * Create the socket and Bind our local address so that any
	 * client can send to us.
	 */

	if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		err_dump("net_init: can't create stream socket");

	if (bind(sockfd, (struct sockaddr *) &tcp_srv_addr,
					sizeof(tcp_srv_addr)) < 0)
		err_dump("net_init: can't bind local address");

	/*
	 * And set the listen parameter, telling the system that we're
	 * ready  to accept incoming connection requests.
	 */

	listen(sockfd, 5);
}

/*
 * Initiate the server's end.
 * We are passed a flag that says whether or not we were started
 * by a "master daemon," such as the inetd program under 4.3BSD.
 * A master daemon will have already waited for a message to arrive
 * for us, and will have already set up the connection to the client.
 * If we weren't started by a master daemon, then we must wait for a
 * client's request to arrive.
 */

int
net_open(inetdflag)
int	inetdflag;	/* true if inetd started us */
{
	register int	newsockfd, childpid, nbytes;
	int		clilen, on;

	on = 1;

	if (inetdflag) {
#ifdef	BSD		/* assumes 4.3BSD inetd */
		/*
		 * When we're fired up by inetd under 4.3BSD, file
		 * descriptors 0, 1 and 2 are sockets to the client.
		 */

		sockfd = 0;	/* descriptor for net_recv() to read from */

		return(0);	/* done */

#endif	/* BSD inetd specifics */
	}

	/*
	 * For the concurrent server that's not initiated by inetd,
	 * we have to wait for a connection request to arrive,
	 * then fork a child to handle the client's request.
	 * Beware that the accept() can be interrupted, such as by
	 * a previously spawned child process that has terminated
	 * (for which we caught the SIGCLD signal).
	 */

again:
	clilen = sizeof(tcp_cli_addr);
	newsockfd = accept(sockfd, (struct sockaddr *) &tcp_cli_addr, &clilen);
	if (newsockfd < 0) {
		if (errno == EINTR) {
			errno = 0;
			goto again;	/* probably a SIGCLD that was caught */
		}
		err_dump("accept error");
	}

	/*
	 * Fork a child process to handle the client's request.
	 * The parent returns the child pid to the caller, which is
	 * probably a concurrent server that'll call us again, to wait
	 * for the next client request to this well-known port.
	 */

	if ( (childpid = fork()) < 0)
		err_dump("server can't fork");

	else if (childpid > 0) {		/* parent */
		close(newsockfd);	/* close new connection */
		return(childpid);	/* and return */
	}

	/*
	 * Child process continues here.
	 * First close the original socket so that the parent
	 * can accept any further requests that arrive there.
	 * Then set "sockfd" in our process to be the descriptor that
	 * we are going to process.
	 */

	close(sockfd);
	sockfd = newsockfd;

	return(0);		/* return to process the connection */
}

#endif	/* SERVER */
