/*
 * Return a string containing some additional information after a
 * host name or address lookup error - gethostbyname() or gethostbyaddr().
 */

int	h_errno;		/* host error number */
int	h_nerr;			/* # of error message strings */
char	*h_errlist[];		/* the error message table */

char *
host_err_str()
{
	static char	msgstr[200];

	if (h_errno != 0) {
		if (h_errno > 0 && h_errno < h_nerr)
			sprintf(msgstr, "(%s)", h_errlist[h_errno]);
		else
			sprintf(msgstr, "(h_errno = %d)", h_errno);
	} else {
		msgstr[0] = '\0';
	}

	return(msgstr);
}
