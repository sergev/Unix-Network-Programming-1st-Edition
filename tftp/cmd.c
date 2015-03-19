/*
 * Command processing functions, one per command.
 * (Only the client side processes user commands.)
 * In alphabetical order.
 */

#include	"cmd.h"

/*
 * ascii
 *
 *	Equivalent to "mode ascii".
 */

cmd_ascii()
{
	modetype = MODE_ASCII;
}

/*
 * binary
 *
 *	Equivalent to "mode binary".
 */

cmd_binary()
{
	modetype = MODE_BINARY;
}

/*
 * connect <hostname> [ <port> ]
 *
 *	Set the hostname and optional port number for future transfers.
 *	The port is the well-known port number of the tftp server on
 *	the other system.  Normally this will default to the value
 *	specified in /etc/services (69).
 */

cmd_connect()
{
	register int	val;

	if (gettoken(hostname) == NULL)
		err_cmd("missing hostname");

	if (gettoken(temptoken) == NULL)
		return;
	val = atoi(temptoken);
	if (val < 0)
		err_cmd("invalid port number");
	port = val;
}

/*
 * exit
 */

cmd_exit()
{
	exit(0);
}

/*
 * get <remotefilename> <localfilename>
 *
 *	Note that the <remotefilename> may be of the form <host>:<filename>
 *	to specify the host also.
 */

cmd_get()
{
	char	remfname[MAXFILENAME], locfname[MAXFILENAME];
	char	*index();

	if (gettoken(remfname) == NULL)
		err_cmd("the remote filename must be specified");
	if (gettoken(locfname) == NULL)
		err_cmd("the local filename must be specified");

	if (index(locfname, ':') != NULL)
		err_cmd("can't have 'host:' in local filename");

	striphost(remfname, hostname);	/* check for "host:" and process */
	if (hostname[0] == 0)
		err_cmd("no host has been specified");

	do_get(remfname, locfname);
}

/*
 * help
 */

cmd_help()
{
	register int	i;

	for (i = 0; i < ncmds; i++) {
		printf("  %s\n", commands[i].cmd_name);
	}
}

/*
 * mode ascii
 * mode binary
 *
 *	Set the mode for file transfers.
 */

cmd_mode()
{
	if (gettoken(temptoken) == NULL) {
		err_cmd("a mode type must be specified");
	} else {
		if (strcmp(temptoken, "ascii") == 0)
			modetype = MODE_ASCII;
		else if (strcmp(temptoken, "binary") == 0)
			modetype = MODE_BINARY;
		else
			err_cmd("mode must be 'ascii' or 'binary'");
	}
}

/*
 * put <localfilename> <remotefilename>
 *
 *	Note that the <remotefilename> may be of the form <host>:<filename>
 *	to specify the host also.
 */

cmd_put()
{
	char	remfname[MAXFILENAME], locfname[MAXFILENAME];

	if (gettoken(locfname) == NULL)
		err_cmd("the local filename must be specified");
	if (gettoken(remfname) == NULL)
		err_cmd("the remote filename must be specified");

	if (index(locfname, ':') != NULL)
		err_cmd("can't have 'host:' in local filename");

	striphost(remfname, hostname);	/* check for "host:" and process */
	if (hostname[0] == 0)
		err_cmd("no host has been specified");

	do_put(remfname, locfname);
}

/*
 * Show current status.
 */

cmd_status()
{
	if (connected)
		printf("Connected\n");
	else
		printf("Not connected\n");

	printf("mode = ");
	switch (modetype) {
	case MODE_ASCII:	printf("netascii");		break;
	case MODE_BINARY:	printf("octet (binary)");	break;
	default:
		err_dump("unknown modetype");
	}

	printf(", verbose = %s", verboseflag ? "on" : "off");
	printf(", trace = %s\n", traceflag ? "on" : "off");
}

/*
 * Toggle debug mode.
 */

cmd_trace()
{
	traceflag = !traceflag;
}

/*
 * Toggle verbose mode.
 */

cmd_verbose()
{
	verboseflag = !verboseflag;
}
