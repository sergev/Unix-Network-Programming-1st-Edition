/*
 * Initialize the external variables.
 * Some link editors (on systems other than UNIX) require this.
 */

#include	"defs.h"

char	hostname[MAXHOSTNAMELEN]	= "hsi";	/* default host */
char	printername[MAXHOSTNAMELEN]	= "lp";		/* default printer */
int	debugflag			= 0;
