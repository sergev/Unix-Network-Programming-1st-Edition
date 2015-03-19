/*
 * Definitions for UNIX domain stream and datagram client/server programs.
 */

#include	<stdio.h>
#include	<sys/types.h>
#include	<sys/socket.h>
#include	<sys/un.h>

#define	UNIXSTR_PATH	"./s.unixstr"
#define	UNIXDG_PATH	"./s.unixdg"
#define	UNIXDG_TMP	"/tmp/dg.XXXXXX"

char	*pname;
