/*
 * Definitions for SPP and IDP client/server programs.
 */

#include	<stdio.h>
#include	<sys/types.h>
#include	<sys/socket.h>
#include	<netns/ns.h>

#define	SPP_SERV_ADDR	"123:02.07.01.00.a1.62:6001"
#define	IDP_SERV_ADDR	"123:02.07.01.00.a1.62:6000"
				/* <netid>:<hostid>:<port> */
#define	SERV_SPP_PORT	6001
#define	SERV_IDP_PORT	6000

char		*pname;
struct ns_addr	ns_addr();	/* BSD library routine */
