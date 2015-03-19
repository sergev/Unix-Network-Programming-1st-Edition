/*
 * Send files to a Berkeley (BSD) line printer daemon.
 * This is done using a TCP connection, following the protocol
 * inherent (and undocumented) in the BSD printer daemon (/usr/lib/lpd).
 *
 * This program differs from the normal BSD lpr(1) command in the
 * following ways.  The BSD lpr command writes the files to be
 * printed into the spooling directory, and then notifies the
 * line printer daemon (lpd) that the files are there (using a
 * UNIX domain socket message).  The lpd program then sees that
 * the files get printed at some time.  If the files are to be
 * printed on another host's line printer, then the printer daemon
 * will contact the daemon on the other host (using an Internet
 * TCP socket) and will transfer the file to the other daemon,
 * who will then see to it that the file gets printed.
 * This program, however, acts like a UNIX printer daemon (lpd)
 * as we assume the files are to be printed on another system.
 * We go ahead and contact the UNIX lpd program on the remote host
 * (using an Internet TCP socket) and send the files to that
 * daemon for printing.
 *
 * There are three functions in this file that main() calls:
 *	send_start() - called once, before the first file
 *	send_file()  - called for every file that can be opened by main
 *	send_done()  - called at the end
 */

#include	"defs.h"
#include	"systype.h"

#include	<pwd.h>
#include	<sys/stat.h>

/*
 * Variables specific to this file.
 */

static FILE	*cfp;			/* file pointer for control file */
static long	cfilesize;		/* size of cfile, in bytes */
static char	myhostname[MAXHOSTNAMELEN]; /* name of host running lpr */
static char	username[MAXHOSTNAMELEN];   /* name of user running lpr */
static char	cfname[MAXFILENAME];	/* name of "cf" file */
static char	dfname[MAXFILENAME];	/* name of "df" file */
static char	buf[MAXLINE];		/* temp buffer */
static int	sockfd;			/* network connection */
static int	seqnum;			/* seq#, set to same value for now */

long	get_size();

/*
 * Start things up.
 */

send_start()
{
	register int	uid;
	struct passwd	*pw, *getpwuid();

	DEBUG2("send_start: host = %s, printer = %s", hostname, printername);

						/* we need a reserved port */
	if ( (sockfd = tcp_open(hostname, LPR_SERVICE, -1)) < 0)
		err_quit("can't connect to service: %s on host: %s",
						LPR_SERVICE, hostname);

	/*
	 * If we got the reserved port, then we're either running as
	 * root, or the program is set-user-ID root.  In the latter case,
	 * the only reason we need to be set-user-ID root is to bind the
	 * reserved port, so we can now go back to being the "real"
	 * user who executed this program.  We really need to do
	 * this anyway, to assure we can't read files as root
	 * that the user doesn't have normal access to.
	 */

	setuid(getuid());

	/*
	 * Get the name of the local host.
	 */

	if (gethostname(myhostname, MAXHOSTNAMELEN) < 0)
		err_dump("gethostname error");

	/*
	 * Get the name of the user executing this program.
	 */

	uid = getuid();
	if ( (pw = getpwuid(uid)) == NULL)
		err_quit("getpwuid failed, uid = %d; who are you", uid);
	strcpy(username, pw->pw_name);

	/*
	 * We must insert a 3-digit sequence number into the filenames
	 * that we're creating to send to the server.  This is to
	 * distinguish between successive files that are sent to the
	 * same server for the same printer.
	 */

	seqnum = get_seqno();

	sprintf(cfname, "cfA%03d%s", seqnum, myhostname);
	sprintf(dfname, "dfA%03d%s", seqnum, myhostname);
	DEBUG2("cfname = %s, dfname = %s", cfname, dfname);

	/*
	 * Create the control file and open it for writing.
	 */

	if ( (cfp = fopen(cfname, "w")) == NULL)
		err_dump("can't open control file: %s for writing", cfname);
	cfilesize = 0L;

	/*
	 * Initialize the control file by inserting the following lines:
	 *
	 *	H<hostname>	(host on which the lpr was executed)
	 *	P<username>	(person executing the lpr)
	 *	J<jobname>	(for the banner page)
	 *	C<classname>	(for the banner page)
	 *	L<username>	(literal value for banner page)
	 *
	 * Then, for each file to be printed, send_file() will add
	 * the following three lines:
	 *
	 *	f<df_filename>	(name of text file to print)
	 *	U<df_filename>	(to unlink the file after printing)
	 *	N<filename>	(real name of the file, used by lpq)
	 *
	 * The <df_filename> is of the form "df[A-Z]nnn<host>" where
	 * "nnn" is the 3-digit sequence number from this host,
	 * and "<host>" is the name of the host on which the lpr
	 * was executed.
	 */

	add_H();
	add_P();
	add_C();
	add_L();

	/*
	 * Send a line to the print server telling it we want
	 * to send it some files to print, and specifying the
	 * printer to be used.
	 */

	sprintf(buf, "%c%s\n", '\002', printername);
	if (writen(sockfd, buf, strlen(buf)) != strlen(buf))
		err_dump("writen error");

	if (readn(sockfd, buf, 1) != 1)
		err_dump("readn error");
	if (buf[0] != '\0') {
		if (readline(sockfd, &buf[1], MAXLINE-1) > 0)
		   err_quit("error, server returned: %s", buf);
		else
		   err_dump("didn't get ACK from server, got 0x%02x", buf[0]);
	}
}

/*
 * Send a single file.
 * This function is called by main once for every file to be printed.
 * Main has already opened the file for reading, but it still passes
 * us the actual filename from the command line, so that we can
 * use the filename for identifying the file to the server.
 */

send_file(filename, fp)
char	*filename;	/* filename from command line, or "-stdin" */
FILE	*fp;		/* file pointer on which file is open for reading */
{
	static int	filecount = 0;
	register char	*ptr;
	char		*rindex();

	/*
	 * We don't currently handle standard input.  To do so requires
	 * that we copy stdin to a temporary file, so that we can get
	 * its size in bytes.  We have to know the file's size before
	 * we send it to the server.
	 */

	if (strcmp(filename, "-stdin") == 0) {
		err_ret("can't currently print standard input");
		return;
	}

	filecount++;

	/*
	 * First strip any leading directory names off the filename.
	 * This is to get the base filename for the job banner.
	 */

	if ( (ptr = rindex(filename, '/')) != NULL) {
		filename = ptr + 1;
	}

	/*
	 * If this is the first file, set the Job Class on the banner
	 * page to the filename.
	 */

	if (filecount == 1)
		add_J(filename);
	else
		dfname[2]++;	/* A, B, C, ... */

	/*
	 * Add the 'f', 'U' and 'N' lines to the control file.
	 */

	add_f(dfname);
	add_U(dfname);
	add_N(filename);

	DEBUG2("send_file: %s, dfname = %s", filename, dfname);

	xmit_file(filename, fp, dfname, '\003');
				/* transmit file across network */
}

/*
 * All done with the user's files.
 * Now we must transmit the control file that we've been building
 * to the other side.
 */

send_done()
{
	fclose(cfp);

	if ( (cfp = fopen(cfname, "r")) == NULL)
		err_dump("can't reopen cfile for reading");

	xmit_file("-cfile", cfp, cfname, '\002');

	fclose(cfp);

	/*
	 * We're done with the control file, so delete it.
	 * (Don't unlink if debugflag is 1, assuming we're debugging.)
	 */

	if (debugflag == 0 && unlink(cfname) < 0)
		err_dump("can't unlink control file: %s", cfname);

	close(sockfd);
}

/*
 * Transmit one file to the server.
 * This routine is used to send both the actual text files (data files)
 * that the user wants printed, and to send the control file (cfile)
 * that we build up as we send the data files.
 * The only difference between transmitting these two types of files
 * is the first byte of the transmission (002 for the cfile and 003
 * for the dfiles).
 */

xmit_file(filename, fp, fname, xmittype)
char	*filename;	/* name from command line, or "-stdin" or "-cfile" */
FILE	*fp;
char	*fname;		/* the cfname or dfname */
char	xmittype;	/* '\002' or '\003' */
{
	register long	size;

	/*
	 * We have to get the exact size of the file in bytes
	 * to send to the server, so that it knows how much
	 * data to read from the net.
	 */

	size = get_size(filename, fp);
	DEBUG2("xmit_file: %s, size = %ld", filename, size);

	/*
	 * Send a line to the print server giving the type of
	 * file, the exact size of the file in bytes,
	 * and the name of the file (its dfname, not its actual
	 * name).
	 */

	sprintf(buf, "%c%ld %s\n", xmittype, size, fname);
	if (writen(sockfd, buf, strlen(buf)) != strlen(buf))
		err_dump("writen error");
	if (readn(sockfd, buf, 1) != 1)
		err_dump("readn error");
	if (buf[0] != '\0')
		err_dump("didn't get an ACK from server, got 0x%02x", buf[0]);

	/*
	 * Now send the actual file itself.
	 */

	copyfile(fp);

	/*
	 * Write a byte of zero to the server, and wait for
	 * a byte of zero to be returned from the server,
	 * telling us all is OK (I'm OK, you're OK).
	 */

	if (writen(sockfd, "", 1) != 1)
		err_dump("writen error");
	if (readn(sockfd, buf, 1) != 1)
		err_dump("readn error");
	if (buf[0] != '\0')
		err_dump("didn't get an ACK from server, got 0x%02x", buf[0]);
}

/*
 * Copy a file to the network.
 * We read the file using standard i/o, one line at a time,
 * and write the data to the network one line at a time.
 */

copyfile(fp)
FILE	*fp;
{
	register int	len;
	char		line[MAXLINE];

	while (fgets(line, MAXLINE, fp) != NULL) {
		len = strlen(line);
		if (writen(sockfd, line, len) != len)
			err_dump("writen error");
	}

	if (ferror(fp))
		err_dump("read error from fgets");
}

/*
 * Determine the exact size of a file.
 * Under UNIX this is easy - we just call the fstat() system call.
 * Under other operating systems it is harder, since they may not use
 * exactly one character to represent a newline.
 */

long
get_size(filename, fp)
char	*filename;
FILE	*fp;
{
	struct stat	statbuff;

	if (fstat(fileno(fp), &statbuff) < 0)
		err_dump("can't fstat");

	return(statbuff.st_size);
}

add_H()
{
	fprintf(cfp, "H%s\n", myhostname);
	cfilesize += strlen(myhostname) + 2;
}

add_P()
{
	fprintf(cfp, "P%s\n", username);
	cfilesize += strlen(username) + 2;
}

/*
 * We add the Job Class when the first file is processed.
 */

add_J(filename)
char	*filename;
{
	fprintf(cfp, "J%s\n", filename);
	cfilesize += strlen(filename) + 2;
}

add_C()
{
	fprintf(cfp, "C%s\n", myhostname);
	cfilesize += strlen(myhostname) + 2;
				/* just use this host's name */
}

add_L()
{
	fprintf(cfp, "L%s\n", username);
	cfilesize += strlen(username) + 2;
}

add_f(dfname)
char	*dfname;
{
	fprintf(cfp, "f%s\n", dfname);
	cfilesize += strlen(dfname) + 2;
}

add_U(dfname)
char	*dfname;
{
	fprintf(cfp, "U%s\n", dfname);
	cfilesize += strlen(dfname) + 2;
}

add_N(filename)
char	*filename;
{
	fprintf(cfp, "N%s\n", filename);
	cfilesize += strlen(filename) + 2;
}
