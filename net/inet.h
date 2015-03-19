/*
 * Definitions for TCP and UDP client/server programs.
 */

#include	<stdio.h>
#include	<sys/types.h>
#include	<sys/socket.h>
#include	<netinet/in.h>
#include	<arpa/inet.h>

#define	SERV_UDP_PORT	6000
#define	SERV_TCP_PORT	6000
#define	SERV_HOST_ADDR	"192.43.235.6"	/* host addr for server */

char	*pname;
