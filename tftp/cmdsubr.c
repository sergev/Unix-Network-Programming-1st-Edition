/*
 * Miscellaneous functions for user command processing.
 */

#include	"cmd.h"

	/* all of the following functions are in cmd.c */
int	cmd_ascii(), cmd_binary(), cmd_connect(), cmd_exit(),
	cmd_get(), cmd_help(), cmd_mode(), cmd_put(),
	cmd_status(), cmd_trace(), cmd_verbose();

Cmds	commands[] = {	/* keep in alphabetical order for binary search */
		"?",		cmd_help,
		"ascii",	cmd_ascii,
		"binary",	cmd_binary,
		"connect",	cmd_connect,
		"exit",		cmd_exit,
		"get",		cmd_get,
		"help",		cmd_help,
		"mode",		cmd_mode,
		"put",		cmd_put,
		"quit", 	cmd_exit,
		"status",	cmd_status,
		"trace",	cmd_trace,
		"verbose",	cmd_verbose,
};
#define	NCMDS	(sizeof(commands) / sizeof(Cmds))

int	ncmds = NCMDS;

static char	line[MAXLINE] = { 0 };
static char	*lineptr = NULL;

/*
 * Fetch the next command line.
 * For interactive use or batch use, the lines are read from a file.
 *
 * Return 1 if OK, else 0 on error or end-of-file.
 */

int
getline(fp)
FILE	*fp;
{
	if (fgets(line, MAXLINE, fp) == NULL)
		return(0);		/* error or end-of-file */
	lineptr = line;

	return(1);
}

/*
 * Fetch the next token from the input stream.
 * We use the line that was set up in the most previous call to
 * getline().
 *
 * Return a pointer to the token (the argument), or NULL if no more exist.
 */

char *
gettoken(token)
char	token[];
{
	register int	c;
	register char	*tokenptr;

	while ((c = *lineptr++) == ' ' || c == '\t')
		;		/* skip leading white space */

	if (c == '\0' || c == '\n')
		return(NULL);	/* nothing there */

	tokenptr = token;
	*tokenptr++ = c;	/* first char of token */

	/*
	 * Now collect everything up to the next space, tab, newline, or null.
	 */

	while ((c = *lineptr++) != ' ' && c != '\t' && c != '\n' && c != '\0')
		*tokenptr++ = c;

	*tokenptr = 0;		/* null terminate token */
	return(token);
}

/*
 * Verify that there aren't any more tokens left on a command line.
 */

checkend()
{
	if (gettoken(temptoken) != NULL)
		err_cmd("trailing garbage");
}

/*
 * Execute a command.
 * Call the appropriate function.  If all goes well, that function will
 * return, otherwise that function may call an error handler, which will
 * call longjmp() and branch back to the main command processing loop.
 */

docmd(cmdptr)
char	*cmdptr;
{
	register int	i;

	if ( (i = binary(cmdptr, ncmds)) < 0)
		err_cmd(cmdptr);

	(*commands[i].cmd_func)();

	checkend();
}

/*
 * Perform a binary search of the command table
 * to see if a given token is a command.
 */

binary(word, n)
char	*word;
int	n;
{
	register int	low, high, mid, cond;

	low  = 0;
	high = n - 1;
	while (low <= high) {
		mid = (low + high) / 2;
		if ( (cond = strcmp(word, commands[mid].cmd_name)) < 0)
			high = mid - 1;
		else if (cond > 0)
			low = mid + 1;
		else
			return(mid);	/* found it, return index in array */
	}
	return(-1);	/* not found */
}

/*
 * Take a "host:file" character string and separate the "host"
 * portion from the "file" portion.
 */

striphost(fname, hname)
char	*fname;		/* input:  "host:file" or just "file" */
char	*hname;		/* store "host" name here, if present */
{
	char		*index();
	register char	*ptr1, *ptr2;

	if ( (ptr1 = index(fname, ':')) == NULL)
		return;		/* there is not a "host:" present */

	/*
	 * Copy the entire "host:file" into the hname array,
	 * then replace the colon with a null byte.
	 */

	strcpy(hname, fname);
	ptr2 = index(hname, ':');
	*ptr2 = 0;	/* null terminates the "host" string */

	/*
	 * Now move the "file" string left in the fname array,
	 * removing the "host:" portion.
	 */

	strcpy(fname, ptr1 + 1);	/* ptr1 + 1 to skip over the ':' */
}

/*
 * User command error.
 * Print out the command line too, for information.
 */

err_cmd(str)
char	*str;		/* may be a 0-length string, i.e., "" */
{
	fprintf(stderr, "%s: '%s' command error", pname, command);
	if (strlen(str) > 0)
		fprintf(stderr, ": %s", str);
	fprintf(stderr, "\n");
	fflush(stderr);

	longjmp(jmp_mainloop, 1);	/* 1 -> not a timeout, we've already
					   printed our error message */
}
