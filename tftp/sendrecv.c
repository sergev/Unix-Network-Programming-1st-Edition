/*
 * Send and receive packets.
 */

#include	"defs.h"
#include	<sys/stat.h>
#include	<ctype.h>

#ifdef	CLIENT

/*
 * Send a Read-Request or a Write-Request to the other system.
 * These two packets are only sent by the client to the server.
 * This function is called when either the "get" command or the
 * "put" command is executed by the user.
 */

send_RQ(opcode, fname, mode)
int	opcode;		/* OP_RRQ or OP_WRQ */
char	*fname;
int	mode;
{
	register int	len;
	char		*modestr;

	DEBUG2("sending RRQ/WRQ for %s, mode = %d", fname, mode);

	stshort(opcode, sendbuff);

	strcpy(sendbuff+2, fname);
	len = 2 + strlen(fname) + 1;	/* +1 for null byte at end of fname */

	switch(mode) {
	case MODE_ASCII:	modestr = "netascii";	break;
	case MODE_BINARY:	modestr = "octet";	break;
	default:
		err_dump("unknown mode");
	}
	strcpy(sendbuff + len, modestr);
	len += strlen(modestr) + 1;	/* +1 for null byte at end of modestr */

	sendlen = len;
	net_send(sendbuff, sendlen);
	op_sent = opcode;
}

/*
 * Error packet received in response to an RRQ or a WRQ.
 * Usually means the file we're asking for on the other system
 * can't be accessed for some reason.  We need to print the
 * error message that's returned.
 * Called by finite state machine.
 */

int
recv_RQERR(ptr, nbytes)
char	*ptr;		/* points just past received opcode */
int	nbytes;		/* doesn't include received opcode */
{
	register int	ecode;

	ecode = ldshort(ptr);
	ptr += 2;
	nbytes -= 2;
	ptr[nbytes] = 0;	/* assure it's null terminated ... */

	DEBUG2("ERROR received, %d bytes, error code %d", nbytes, ecode);

	fflush(stdout);
	fprintf(stderr, "Error# %d: %s\n", ecode, ptr);
	fflush(stderr);

	return(-1);	/* terminate finite state loop */
}

#endif	/* CLIENT */

/*
 * Send an acknowledgment packet to the other system.
 * Called by the recv_DATA() function below and also called by
 * recv_WRQ().
 */

send_ACK(blocknum)
int	blocknum;
{
	DEBUG1("sending ACK for block# %d", blocknum);

	stshort(OP_ACK, sendbuff);
	stshort(blocknum, sendbuff + 2);

	sendlen = 4;
	net_send(sendbuff, sendlen);

#ifdef	SORCERER
	/*
	 * If you want to see the Sorcerer's Apprentice syndrome,
	 * #define SORCERER, then run this program as the client and
	 * get a file from a server that doesn't have the bug fixed
	 * (such as the 4.3BSD version).
	 * Turn on the trace option, and you'll see the duplicate
	 * data packets sent by the broken server, starting with
	 * block# 2.  Yet when the transfer is complete, you'll find
	 * the file was received correctly.
	 */

	if (blocknum == 1)
		net_send(sendbuff, sendlen);	/* send the first ACK twice */
#endif

	op_sent = OP_ACK;
}

/*
 * Send data to the other system.
 * The data must be stored in the "sendbuff" by the caller.
 * Called by the recv_ACK() function below.
 */

send_DATA(blocknum, nbytes)
int	blocknum;
int	nbytes;		/* #bytes of actual data to send */
{
	DEBUG2("sending %d bytes of DATA with block# %d", nbytes, blocknum);

	stshort(OP_DATA, sendbuff);
	stshort(blocknum, sendbuff + 2);

	sendlen = nbytes + 4;
	net_send(sendbuff, sendlen);
	op_sent = OP_DATA;
}

/*
 * Data packet received.  Send an acknowledgment.
 * Called by finite state machine.
 * Note that this function is called for both the client and the server.
 */

int
recv_DATA(ptr, nbytes)
register char	*ptr;		/* points just past received opcode */
register int	nbytes;		/* doesn't include received opcode */
{
	register int	recvblknum;

	recvblknum = ldshort(ptr);
	ptr += 2;
	nbytes -= 2;

	DEBUG2("DATA received, %d bytes, block# %d", nbytes, recvblknum);

	if (nbytes > MAXDATA)
		err_dump("data packet received with length = %d bytes", nbytes);

	if (recvblknum == nextblknum) {
		/*
		 * The data packet is the expected one.
		 * Increment our expected-block# for the next packet.
		 */

		nextblknum++;
		totnbytes += nbytes;

		if (nbytes > 0) {
			/*
			 * Note that the final data packet can have a
			 * data length of zero, so we only write the
			 * data to the local file if there is data.
			 */

			file_write(localfp, ptr, nbytes, modetype);
		}
#ifdef	SERVER
		/*
		 * If the length of the data is between 0-511, this is
		 * the last data block.  For the server, here's where
		 * we have to close the file.  For the client, the
		 * "get" command processing will close the file.
		 */

		if (nbytes < MAXDATA)
			file_close(localfp);
#endif

	} else if (recvblknum < (nextblknum - 1)) {
		/*
		 * We've just received data block# N (or earlier, such as N-1,
		 * N-2, etc.) from the other end, but we were expecting data
		 * block# N+2.  But if we were expecting N+2 it means we've
		 * already received N+1, so the other end went backwards from
		 * N+1 to N (or earlier).  Something is wrong.
		 */

		err_dump("recvblknum < nextblknum - 1");

	} else if (recvblknum > nextblknum) {
		/*
		 * We've just received data block# N (or later, such as N+1,
		 * N+2, etc.) from the other end, but we were expecting data
		 * block# N-1.  But this implies that the other end has
		 * received an ACK for block# N-1 from us.  Something is wrong.
		 */

		err_dump("recvblknum > nextblknum");
	}

	/*
	 * The only case not handled above is "recvblknum == (nextblknum - 1)".
	 * This means the other end never saw our ACK for the last data
	 * packet and retransmitted it.  We just ignore the retransmission
	 * and send another ACK.
	 *
	 * Acknowledge the data packet.
	 */

	send_ACK(recvblknum);

	/*
	 * If the length of the data is between 0-511, we've just
	 * received the final data packet, else there is more to come.
	 */

	return( (nbytes == MAXDATA) ? 0 : -1 );
}

/*
 * ACK packet received.  Send some more data.
 * Called by finite state machine.  Also called by recv_RRQ() to
 * start the transmission of a file to the client.
 * Note that this function is called for both the client and the server.
 */

int
recv_ACK(ptr, nbytes)
register char	*ptr;		/* points just past received opcode */
register int	nbytes;		/* doesn't include received opcode */
{
	register int	recvblknum;

	recvblknum = ldshort(ptr);
	if (nbytes != 2)
		err_dump("ACK packet received with length = %d bytes",
						nbytes + 2);

	DEBUG1("ACK received, block# %d", recvblknum);

	if (recvblknum == nextblknum) {
		/*
		 * The received acknowledgment is for the expected data
		 * packet that we sent.
		 * Fill the transmit buffer with the next block of data
		 * to send.
		 * If there's no more data to send, then we might be
		 * finished.  Note that we must send a final data packet
		 * containing 0-511 bytes of data.  If the length of the
		 * last packet that we sent was exactly 512 bytes, then we
		 * must send a 0-length data packet.
		 */

		if ( (nbytes = file_read(localfp, sendbuff + 4,
						MAXDATA, modetype)) == 0) {
			if (lastsend < MAXDATA)
				return(-1);	/* done */
			/* else we'll send nbytes=0 of data */
		}

		lastsend = nbytes;
		nextblknum++;		/* incr for this new packet of data */
		totnbytes += nbytes;
		send_DATA(nextblknum, nbytes);

		return(0);

	} else if (recvblknum < (nextblknum - 1)) {
		/*
		 * We've just received the ACK for block# N (or earlier, such
		 * as N-1, N-2, etc) from the other end, but we were expecting
		 * the ACK for block# N+2.  But if we're expecting the ACK for
		 * N+2 it means we've already received the ACK for N+1, so the
		 * other end went backwards from N+1 to N (or earlier).
		 * Something is wrong.
		 */

		err_dump("recvblknum < nextblknum - 1");

	} else if (recvblknum > nextblknum) {
		/*
		 * We've just received the ACK for block# N (or later, such
		 * as N+1, N+2, etc) from the other end, but we were expecting
		 * the ACK for block# N-1.  But this implies that the other
		 * end has already received data block# N-1 from us.
		 * Something is wrong.
		 */

		err_dump("recvblknum > nextblknum");

	} else {
		/*
		 * Here we have "recvblknum == (nextblknum - 1)".
		 * This means we received a duplicate ACK.  This means either:
		 * (1) the other side never received our last data packet;
		 * (2) the other side's ACK got delayed somehow.
		 *
		 * If we were to retransmit the last data packet, we would start
		 * the "Sorcerer's Apprentice Syndrome."  We'll just ignore this
		 * duplicate ACK, returning to the FSM loop, which will initiate
		 * another receive.
		 */

		return(0);
	}
	/* NOTREACHED */
}

#ifdef	SERVER

/*
 * RRQ packet received.
 * Called by the finite state machine.
 * This (and receiving a WRQ) are the only ways the server gets started.
 */

int
recv_RRQ(ptr, nbytes)
char	*ptr;
int	nbytes;
{
	char	ackbuff[2];

	recv_xRQ(OP_RRQ, ptr, nbytes);	/* verify the RRQ packet */

	/*
	 * Set things up so we can just call recv_ACK() and pretend we
	 * received an ACK, so it'll send the first data block to the
	 * client.
	 */

	lastsend = MAXDATA;
	stshort(0, ackbuff);	/* pretend its an ACK of block# 0 */

	recv_ACK(ackbuff, 2);	/* this sends data block# 1 */

	return(0);	/* the finite state machine takes over from here */
}

/*
 * WRQ packet received.
 * Called by the finite state machine.
 * This (and receiving an RRQ) are the only ways the server gets started.
 */

int
recv_WRQ(ptr, nbytes)
char	*ptr;
int	nbytes;
{
	recv_xRQ(OP_WRQ, ptr, nbytes);	/* verify the WRQ packet */

	/*
	 * Call send_ACK() to acknowledge block# 0, which will cause
	 * the client to send data block# 1.
	 */

	nextblknum = 1;
	send_ACK(0);

	return(0);	/* the finite stat machine takes over from here */
}

/*
 * Process an RRQ or WRQ that has been received.
 * Called by the 2 routines above.
 */

int
recv_xRQ(opcode, ptr, nbytes)
int		opcode;		/* OP_RRQ or OP_WRQ */
register char	*ptr;		/* points just past received opcode */
register int	nbytes;		/* doesn't include received opcode */
{
	register int	i;
	register char	*saveptr;
	char		filename[MAXFILENAME], dirname[MAXFILENAME],
			mode[MAXFILENAME];
	struct stat	statbuff;

	/*
	 * Assure the filename and mode are present and
	 * null-terminated.
	 */

	saveptr = ptr;		/* points to beginning of filename */
	for (i = 0; i < nbytes; i++)
		if (*ptr++ == '\0')
			goto FileOK;
	err_dump("Invalid filename");

FileOK:
	strcpy(filename, saveptr);
	saveptr = ptr;		/* points to beginning of Mode */

	for ( ; i < nbytes; i++)
		if (*ptr++ == '\0')
			goto ModeOK;
	err_dump("Invalid Mode");

ModeOK:
	strlccpy(mode, saveptr);	/* copy and convert to lower case */

	if (strcmp(mode, "netascii") == 0)
		modetype = MODE_ASCII;
	else if (strcmp(mode, "octet") == 0)
		modetype = MODE_BINARY;
	else
		send_ERROR(ERR_BADOP, "Mode isn't netascii or octet");

	/*
	 * Validate the filename.
	 * Note that as a daemon we might be running with root
	 * privileges.  Since there are no user-access checks with
	 * tftp (as compared to ftp, for example) we will only
	 * allow access to files that are publicly accessible.
	 *
	 * Also, since we're running as a daemon, our home directory
	 * is the root, so any filename must have it's full
	 * pathname specified (i.e., it must begin with a slash).
	 */

	if (filename[0] != '/')
		send_ERROR(ERR_ACCESS, "filename must begin with '/'");

	if (opcode == OP_RRQ) {
		/*
		 * Read request - verify that the file exists
		 * and that it has world read permission.
		 */

		if (stat(filename, &statbuff) < 0)
			send_ERROR(ERR_ACCESS, sys_err_str());
		if ((statbuff.st_mode & (S_IREAD >> 6)) == 0)
			send_ERROR(ERR_ACCESS,
				"File doesn't allow world read permission");

	} else if (opcode == OP_WRQ) {
		/*
		 * Write request - verify that the directory
		 * that the file is being written to has world
		 * write permission.  We've already verified above
		 * that the filename starts with a '/'.
		 */

		char	*rindex();

		strcpy(dirname, filename);
		*(rindex(dirname, '/') + 1) = '\0';
		if (stat(dirname, &statbuff) < 0)
			send_ERROR(ERR_ACCESS, sys_err_str());
		if ((statbuff.st_mode & (S_IWRITE >> 6)) == 0)
			send_ERROR(ERR_ACCESS,
			  "Directory doesn't allow world write permission");

	} else
		err_dump("unknown opcode");

	localfp = file_open(filename, (opcode == OP_RRQ) ? "r" : "w", 0);
	if (localfp == NULL)
		send_ERROR(ERR_NOFILE, sys_err_str());	/* doesn't return */
}

/*
 * Send an error packet.
 * Note that an error packet isn't retransmitted or acknowledged by
 * the other end, so once we're done sending it, we can exit.
 */

send_ERROR(ecode, string)
int	ecode;		/* error code, ERR_xxx from defs.h */
char	*string;	/* some additional info */
			/* can't be NULL, set to "" if empty */
{
	DEBUG2("sending ERROR, code = %d, string = %s", ecode, string);

	stshort(OP_ERROR, sendbuff);
	stshort(ecode, sendbuff + 2);

	strcpy(sendbuff + 4, string);

	sendlen = 4 + strlen(sendbuff + 4) + 1;		/* +1 for null at end */
	net_send(sendbuff, sendlen);

	net_close();

	exit(0);
}

/*
 * Copy a string and convert it to lower case in the process.
 */

strlccpy(dest, src)
register char	*dest, *src;
{
	register char	c;

	while ( (c = *src++) != '\0') {
		if (isupper(c))
			c = tolower(c);
		*dest++ = c;
	}
	*dest = 0;
}

#endif	/* SERVER */
