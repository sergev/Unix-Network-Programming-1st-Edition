/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

/*
 * From the file: "@(#)rcmd.c   5.20 (Berkeley) 1/24/89";
 */

/*
 * Validate a remote user.  This function is called on the server's system.
 *
 * Called by: /etc/rshd (the server for the rcmd() function);
 *	      /bin/login (when invoked by /etc/rlogind for a remote login).
 */

#include	<sys/types.h>
#include	<sys/param.h>
#include	<sys/stat.h>
#include	<stdio.h>
#include	<pwd.h>
#include	<ctype.h>

int	_check_rhosts_file = 1;	/* set to 0 by rlogind and rshd, if they're
				   invoked with -l option */

int					/* return 0 if OK, else -1 */
ruserok(rhost, superuser, cliuname, servuname)
char	*rhost;	      /* "hostname" or "hostname.domain" of client; obtained
			 by caller: gethostbyaddr( getpeername( ) ) */
int	superuser;    /* 1 if caller wants to be root on this (server's) sys */
char	*cliuname;    /* username on client's system */
char	*servuname;   /* username on this (server's) system */
{
	register int	first, hostlen;
	register char	*src, *dst;
	char		clihostname[MAXHOSTNAMELEN];
	register FILE	*hostfp;

	/*
	 * First make a copy of the client's host name, remembering if
	 * it contains a domain suffix.  Also convert everything to
	 * lower case.
	 */

	src = rhost;
	dst = clihostname;
	hostlen = -1;
	while (*src) {
		if (*src == '.') {
			/*
			 * When we hit a period, check if it's the first period,
			 * and if so, save the length of the host name
			 * (i.e., everything up to the first period).
			 */

			if (hostlen == -1)
				hostlen = src - rhost;
			*dst++ = *src++;
		} else {
			*dst++ = isupper(*src) ? tolower(*src++) : *src++;
		}
	}
	*dst = '\0';

	/*
	 * If the caller wants to be the superuser on this system, then all
	 * we check is the file "/.rhosts".  Else we'll check the file
	 * "/etc/hosts.equiv" the first time through the loop below.
	 */

	hostfp = superuser ? (FILE *) 0 : fopen("/etc/hosts.equiv", "r");

	first = 1;
again:
	if (hostfp) {
		if (_validuser(hostfp, clihostname, servuname, cliuname,
								hostlen) == 0) {
			fclose(hostfp);		/* all OK, close file */
			return(0);		/* we're done */
		}
		fclose(hostfp);		/* not valid, close file */
	}
	if (first == 1 && (_check_rhosts_file || superuser)) {
		struct stat	statbuff;
		struct passwd	*pwd;
		char		buff[MAXPATHLEN];

		first = 0;
		if ( (pwd = getpwnam(servuname)) == NULL)
			return(-1);	/* no password file entry */

		strcpy(buff, pwd->pw_dir); /* get home directory of servuname */
		strcat(buff, "/.rhosts");  /* will be "//.rhosts" if root */
		if ( (hostfp = fopen(buff, "r")) == NULL)
			return(-1);	/* can't open user's .rhosts file */

		/*
		 * If the user is not root, then the owner of the .rhosts file
		 * has to be the user.  Also, the .rhosts file can't be
		 * writable by anyone other than the owner.
		 */

		if ((fstat(fileno(hostfp), &statbuff) < 0) ||
		    (statbuff.st_uid != 0 && statbuff.st_uid != pwd->pw_uid) ||
		    (statbuff.st_mode & 022)) {
			fclose(hostfp);
			return(-1);
		}
		goto again;	/* go and call _validuser() */
	}
	return(-1);		/* not a valid user */
}

/*
 * Validate a user.  This is called from the ruserok() function above, and
 * directly by the lpd server.  When the lpd server calls us, it sets both
 * "servuname" and "cliuname" to point to the same string, so that they'll
 * compare as equal.
 */

int				/* return 0 if valid, else -1 */
_validuser(hostfp, clihostname, servuname, cliuname, hostlen)
FILE	*hostfp;	/* FILE pointer to "/etc/hosts.equiv", ".rhosts",
			   or "/etc/hosts.lpd" file */
char	*clihostname;	/* client's "hostname" or "hostname.domain" */
char	*servuname;	/* username on this (server's) system */
char	*cliuname;	/* username on client's system */
int	hostlen;	/* -1 if "hostname"; else its "hostname.domain" and this
			   is the #chars in hostname */
{
	register char	*ptr;
	char		*user;
	char		hostname[MAXHOSTNAMELEN];

	/*
	 * Read and process each line of the file.
	 */

	while (fgets(hostname, sizeof(hostname), hostfp)) {
		ptr = hostname;
		while (*ptr != '\n' && *ptr != ' ' &&
		       *ptr != '\t' && *ptr != '\0') {
				*ptr = isupper(*ptr) ? tolower(*ptr) : *ptr;
				ptr++;
		}
		if (*ptr == ' ' || *ptr == '\t') {
			/*
			 * If the host name was terminated with either a blank
			 * or a tab, then there's a user name following.
			 * We set "user" to point to the first character of
			 * this user name.
			 */

			*ptr++ = '\0';		/* null terminate host name */

			while (*ptr == ' ' || *ptr == '\t')
				ptr++;		/* skip over the white space */
			user = ptr;

			/*
			 * We have to skip to the end of the user name, so that
			 * we can assure it's terminated with a null byte.
			 */

			while (*ptr != '\n' && *ptr != ' ' &&
			       *ptr != '\t' && *ptr != '\0')
					ptr++;
		} else {
			/*
			 * If there's not a user name in the line from the file,
			 * we have "user" point to the null byte that terminates
			 * the host name.  This means the user name is a C null
			 * string.  This fact is used in the strcmp() below.
			 */

			user = ptr;
		}
		*ptr = '\0';	/* null terminate host name or user name */

		/*
		 * If the host fields match (_checkhost() function below), then:
		 * (a) if a user name appeared in the line from the file that
		 *     we processed above, then that user name has to equal the
		 *     user name on the server (servuname).
		 * (b) if no user name appeared in the line from the file, then
		 *     the user name on the server (servuname) has to equal the
		 *     user name on the client (cliuname).
		 */

		if (_checkhost(clihostname, hostname, hostlen) &&
		    (strcmp(cliuname, *user ? user : servuname) == 0) )
			return(0);	/* OK */

		/* else read next line of file */
	}
	return(-1);		/* end-of-file, no match found */
}

/*
 * Validate only the client's host name.
 * We compare the client's host name, either "hostname" or "hostname.domain"
 * to the host name entry from a line of the /etc/hosts.equiv or .rhosts file.
 * If there is not a domain qualifier in the client's host name, then things
 * are simple: the client's hostname has to exactly equal the hostname
 * field from the line of the file.
 *
 * However, domains complicate this routine.  See the comments below.
 */

static char	locdomname[MAXHOSTNAMELEN + 1];
static char	*locdomptr = NULL;
static int	locdomerror = 0;

static int					/* return 1 if OK, else 0 */
_checkhost(clihostname, hostfield, hostlen)
char	*clihostname;	/* client's "hostname" or "hostname.domain" */
char	*hostfield;	/* the hostname field from the file */
int	hostlen;	/* -1 if no ".domain", else length of "hostname" */
{
	register char	*ptr;
	char		*index();

	/*
	 * If there isn't a domain qualifier on the client's hostname,
	 * then just compare the host names.  They have to be equal
	 * to return 1 (OK).
	 */

	if (hostlen == -1)
		return(strcmp(clihostname, hostfield) == 0);	/* 1 if equal */

	/*
	 * There is a domain qualifier on the client's hostname.  If the
	 * client's hostname (everything up to the first period) doesn't equal
	 * the host name in the file, then return an error now.
	 */

	if (strncmp(clihostname, hostfield, hostlen) != 0)
		return(0);	/* not a valid host */

	/*
	 * If the client's "hostname.domain" exactly equals the entry from
	 * the file, then return OK now.
	 */

	if (strcmp(clihostname, hostfield) == 0)
		return(1);	/* valid host */

	/*
	 * If the entry in the file is not "hostname." (a terminating period,
	 * without a domain name), then return an error now.
	 */

	if (*(hostfield + hostlen) != '\0')
		return(0);	/* not a valid host */

	/*
	 * When we get here, the entry from the file is "hostname.",
	 * meaning "use the local domain" as the domain.
	 * If we've already tried to obtain the local domain and encountered
	 * an error, return an error.
	 */

	if (locdomerror == 1)
		return(0);	/* couldn't get local domain name */

	if (locdomptr == NULL) {
		/*
		 * Try once to get the local domain name.
		 */

		if (gethostname(locdomname, sizeof(locdomname)) == -1) {
			locdomerror = 1;	/* system call error */
			return(0);		/* return not valid */
		}
		locdomname[MAXHOSTNAMELEN] = '\0';
				/* assure it's null terminated */

		/*
		 * We got the "localhostname.localdomainmame" string from the
		 * kernel.  Save the pointer to the "locdomainmame" part,
		 * then assure it's lower case.
		 */

		if ( (locdomptr = index(locdomname, '.')) == NULL) {
			locdomerror = 1;	/* humm, no period */
			return(0);		/* return not valid */
		}

		for (ptr = ++locdomptr; *ptr; ptr++)
			if (isupper(*ptr))
				*ptr = tolower(*ptr);
	}

	/*
	 * We know the host names are identical, and the entry in the file was
	 * "hostname.".  So now we compare the client's domain with the local
	 * domain, and if equal, all is OK.
	 */

	return(strcmp(locdomptr, clihostname + hostlen + 1) == 0);
					/* returns 1 if equal, 0 otherwise */
}
