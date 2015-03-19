/*
 * Definitions for more reliable UDP client/server programs.
 */

#include	<stdio.h>
#include	<sys/types.h>
#include	<sys/socket.h>
#include	<netinet/in.h>
#include	<sys/errno.h>
extern int	errno;

#define	HOST		"hsi86"
#define	MYECHO_SERVICE	"myecho"

#define	MAXLINE		 255

char		*pname;
