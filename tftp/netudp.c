/*
 * TFTP network handling for UDP/IP connection.
 */

#include	"netdefs.h"

#include	<netinet/in.h>
#include	<arpa/inet.h>
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

#ifdef	CLIENT

extern struct sockaddr_in	udp_srv_addr;	/* set by udp_open() */
extern struct servent		udp_serv_info;	/* set by udp_open() */
static int			recv_first;


/*
 * Open the network connection.
 */

int
net_open(host, service, port)
char	*host;		/* name of other system to communicate with */
char	*service;	/* name of service being requested */
int	port;		/* if > 0, use as port#, else use value for service */
{
	struct sockaddr_in	addr;

	/*
	 * Call udp_open() to create the socket.  We tell udp_open to
	 * not connect the socket, since we'll receive the first response
	 * from a port that's different from where we send our first
	 * datagram to.
	 */

	if ( (sockfd = udp_open(host, service, port, 1)) < 0)
		return(-1);

	DEBUG2("net_open: host %s, port# %d",
			inet_ntoa(udp_srv_addr.sin_addr),
			ntohs(udp_srv_addr.sin_port));

	strcpy(openhost, host);		/* save the host's name */
	recv_first = 1;			/* flag for net_recv() */

	return(0);
}

/*
 * Close the network connection.
 */

net_close()
{
	DEBUG2("net_close: host = %s, fd = %d", openhost, sockfd);

	close(sockfd);

	sockfd = -1;
}

/*
 * Send a record to the other end.
 * We use the sendto() system call, instead of send(), since the address
 * of the server changes after the first packet is sent.
 */

net_send(buff, len)
char	*buff;
int	len;
{
	register int	rc;

	DEBUG3("net_send: sent %d bytes to host %s, port# %d",
			len, inet_ntoa(udp_srv_addr.sin_addr),
			ntohs(udp_srv_addr.sin_port));

	rc = sendto(sockfd, buff, len, 0, (struct sockaddr *) &udp_srv_addr,
					sizeof(udp_srv_addr));
	if (rc < 0)
		err_dump("send error");
}

/*
 * Receive a record from the other end.
 */

int				/* return #bytes in packet, or -1 on EINTR */
net_recv(buff, maxlen)
char	*buff;
int	maxlen;
{
	register int		nbytes;
	int			fromlen;	/* value-result parameter */
	struct sockaddr_in	from_addr;	/* actual addr of sender */

	fromlen = sizeof(from_addr);
	nbytes = recvfrom(sockfd, buff, maxlen, 0,
			  	(struct sockaddr *) &from_addr, &fromlen);
	/*
	 * The recvfrom() system call can be interrupted by an alarm
	 * interrupt, in case it times out.  We just return -1 if the
	 * system call was interrupted, and the caller must determine
	 * if this is OK or not.
	 */

	if (nbytes < 0) {
		if (errno == EINTR)
			return(-1);
		else
			err_dump("recvfrom error");
	}

	DEBUG3("net_recv: got %d bytes from host %s, port# %d",
		      nbytes, inet_ntoa(from_addr.sin_addr),
		      ntohs(from_addr.sin_port));

	/*
	 * The TFTP client using UDP/IP has a funny requirement.
	 * The problem is that UDP is being used for a
	 * "connection-oriented" protocol, which it wasn't really
	 * designed for.  Rather than tying up a single well-known
	 * port number, the server changes its port after receiving
	 * the first packet from a client.
	 *
	 * The first packet a client sends to the server (an RRQ or a WRQ)
	 * must be sent to its well-known port number (69 for TFTP).
	 * The server is then to choose some other port number for all
	 * subsequent transfers.  The recvfrom() call above will return
	 * the server's current address.  If the port number that we
	 * sent the last packet to (udp_srv_addr.sin_port) is still equal to
	 * the initial well-known port number (udp_serv_info.s_port), then we
	 * must set the server's port for our next transmission to be
	 * the port number from the recvfrom().  See Section 4 of the RFC
	 * where it talks about the TID (= port#).
	 *
	 * Furthermore, after we have determined the port number that
	 * we'll be receiving from, we can verify each datagram to make
	 * certain its from the right place.
	 */

	if (recv_first) {
		/*
		 * This is the first received message.
		 * The server's port should have changed.
		 */

		if (udp_srv_addr.sin_port == from_addr.sin_port)
			err_dump("first receive from port %d",
				 	ntohs(from_addr.sin_port));

		udp_srv_addr.sin_port = from_addr.sin_port;
				/* save the new port# of the server */
		recv_first = 0;

	} else if (udp_srv_addr.sin_port != from_addr.sin_port) {
		err_dump("received from port %d, expected from port %d",
				ntohs(from_addr.sin_port),
				ntohs(udp_srv_addr.sin_port));
	}

	return(nbytes);		/* return the actual length of the message */
}

#endif	/* CLIENT */

#ifdef	SERVER

#include	<sys/ioctl.h>

struct sockaddr_in	udp_srv_addr;	/* server's Internet socket addr */
struct sockaddr_in	udp_cli_addr;	/* client's Internet socket addr */
struct servent		udp_serv_info;	/* from getservbyname() */

static int	recv_nbytes = -1;
static int	recv_first = 0;

extern char	recvbuff[];		/* this is declared in initvars.c */

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

	if ( (sp = getservbyname(service, "udp")) == NULL)
		err_dump("net_init: unknown service: %s/udp", service);

	if (port > 0)
		sp->s_port = htons(port);	/* caller's value */
	udp_serv_info = *sp;			/* structure copy */

	if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		err_dump("net_init: can't create datagram socket");

	/*
	 * Bind our local address so that any client can send to us.
	 */

	bzero((char *) &udp_srv_addr, sizeof(udp_srv_addr));
	udp_srv_addr.sin_family      = AF_INET;
	udp_srv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	udp_srv_addr.sin_port = sp->s_port;

	if (bind(sockfd, (struct sockaddr *) &udp_srv_addr,
					sizeof(udp_srv_addr)) < 0)
		err_dump("net_init: can't bind local address");
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
	register int	childpid, nbytes;
	int		on;

	on = 1;

	if (inetdflag) {
#ifdef	BSD		/* assumes 4.3BSD inetd */
		/*
		 * When we're fired up by inetd under 4.3BSD, file
		 * descriptors 0, 1 and 2 are sockets to the client.
		 * We want to first receive the message that's waiting
		 * for us on the socket, and then close the socket.
		 * This will let inetd go back to waiting for another
		 * request on our "well-known port."
		 */

		sockfd = 0;	/* descriptor for net_recv() to recvfrom() */

		/*
		 * Set the socket to nonblocking, since inetd won't invoke
		 * us unless there's a datagram ready for us to read.
		 */

		if (ioctl(sockfd, FIONBIO, (char *) &on) < 0)
			err_dump("ioctl FIONBIO error");

#endif	/* BSD inetd specifics */

	}

	/*
	 * Now read the first message from the client.
	 * In the inetd case, the message is already here and the call to
	 * net_recv() returns immediately.  In the other case, net_recv()
	 * blocks until a client request arrives.
	 */

	recv_first  =  1;	/* tell net_recv to save the address */
	recv_nbytes = -1;	/* tell net_recv to do the actual read */
	nbytes = net_recv(recvbuff, MAXBUFF);

	/*
	 * Fork a child process to handle the client's request.
	 * In the inetd case, the parent exits, which allows inetd to
	 * handle the next request that arrives to this well-known port
	 * (inetd's wait mode for a datagram socket).
	 * Otherwise the parent returns the child pid to the caller, which
	 * is probably a concurrent server that'll call us again, to wait
	 * for the next client request to this well-known port.
	 */

	if ( (childpid = fork()) < 0)
		err_dump("server can't fork");

	else if (childpid > 0) {	/* parent */
		if (inetdflag)
			exit(0);		/* inetd case; we're done */
		else
			return(childpid);	/* independent server */
	}

	/*
	 * Child process continues here.
	 * First close the socket that is bound to the well-known address:
	 * the parent will handle any further requests that arrive there.
	 * We've already read the message that arrived for us to handle.
	 */

	if (inetdflag) {
		close(0);
		close(1);
		close(2);
	} else {
		close(sockfd);
	}
	errno = 0;		/* in case it was set by a close() */

	/*
	 * Create a new socket.
	 * Bind any local port# to the socket as our local address.
	 * We don't connect(), since net_send() uses the sendto()
	 * system call, specifying the destination address each time.
	 */

	if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		err_dump("net_open: can't create socket");

	bzero((char *) &udp_srv_addr, sizeof(udp_srv_addr));
	udp_srv_addr.sin_family      = AF_INET;
	udp_srv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	udp_srv_addr.sin_port        = htons(0);
	if (bind(sockfd, (char *) &udp_srv_addr, sizeof(udp_srv_addr)) < 0)
		err_dump("net_open: bind error");

	/*
	 * Now we'll set a special flag for net_recv(), so that
	 * the next time it's called, it'll know that the recvbuff[]
	 * already has the received packet in it (from our call to
	 * net_recv() above).
	 */

	recv_nbytes = nbytes;

	return(0);
}

/*
 * Close a socket.
 */

net_close()
{
	DEBUG2("net_close: host = %s, fd = %d", openhost, sockfd);

	close(sockfd);

	sockfd = -1;
}

/*
 * Send a record to the other end.
 * The "struct sockaddr_in cli_addr" specifies the client's address.
 */

net_send(buff, len)
char	*buff;
int	len;
{
	register int	rc;

	DEBUG3("net_send: sent %d bytes to host %s, port# %d",
			len, inet_ntoa(udp_cli_addr.sin_addr),
			ntohs(udp_cli_addr.sin_port));

	rc = sendto(sockfd, buff, len, 0, (struct sockaddr *) &udp_cli_addr,
				sizeof(udp_cli_addr));
	if (rc != len)
		err_dump("sendto error");
}

/*
 * Receive a record from the other end.
 * We're called not only by the user, but also by net_open() above,
 * to read the first datagram after a "connection" is established.
 */

int
net_recv(buff, maxlen)
char	*buff;
int	maxlen;
{
	register int		nbytes;
	int			fromlen;	/* value-result parameter */
	extern int		tout_flag;	/* set by SIGALRM */
	struct sockaddr_in	from_addr;

	if (recv_nbytes >= 0) {
		/*
		 * First message has been handled specially by net_open().
		 * It's already been read into recvbuff[].
		 */

		nbytes = recv_nbytes;
		recv_nbytes = -1;
		return(nbytes);
	}

again:
	fromlen = sizeof(from_addr);
	nbytes = recvfrom(sockfd, buff, maxlen, 0,
				(struct sockaddr *) &from_addr, &fromlen);
	/*
	 * The server can have its recvfrom() interrupted by either an
	 * alarm timeout or by a SIGCLD interrupt.  If it's a timeout,
	 * "tout_flag" will be set and we have to return to the caller
	 * to let them determine if another receive should be initiated.
	 * For a SIGCLD signal, we can restart the recvfrom() ourself.
	 */

	if (nbytes < 0) {
		if (errno == EINTR) {
			if (tout_flag)
				return(-1);

			errno = 0;	/* assume SIGCLD */
			goto again;
		}
		err_dump("recvfrom error");
	}

	DEBUG3("net_recv: got %d bytes from host %s, port# %d",
			nbytes, inet_ntoa(from_addr.sin_addr),
			ntohs(from_addr.sin_port));

	/*
	 * If "recv_first" is set, then we must save the received
	 * address that recvfrom() stored in "from_addr" in the
	 * global "udp_cli_addr".
	 */

	if (recv_first) {
		bcopy((char *) &from_addr, (char *) &udp_cli_addr,
						sizeof(from_addr));
		recv_first = 0;
	}

	/*
	 * Make sure the message is from the expected client.
	 */

	if (udp_cli_addr.sin_port != 0 &&
	    udp_cli_addr.sin_port != from_addr.sin_port)
		err_dump("received from port %d, expected from port %d",
		       ntohs(from_addr.sin_port), ntohs(udp_cli_addr.sin_port));

	return(nbytes);		/* return the actual length of the message */
}

#endif	/* SERVER */
