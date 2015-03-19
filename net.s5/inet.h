/*
 * Definitions for TCP and UDP client/server programs.
 */

#include	<stdio.h>
#include	<fcntl.h>
#include	<tiuser.h>		/* System V R3.2 TLI definitions */
#include	<sys/types.h>
#include	<sys/socket.h>		/* WIN/TCP definitions */
#include	<sys/in.h>		/* WIN/TCP definitions */

#define	DEV_UDP		"/dev/udp"	/* WIN/TCP names */
#define	DEV_TCP		"/dev/tcp"	/* WIN/TCP names */

#define	SERV_UDP_PORT	6000
#define	SERV_TCP_PORT	6000
#define	SERV_HOST_ADDR	"192.43.235.6"	/* host addr for server */

#define	MAXLINE		 255

char	*pname;
