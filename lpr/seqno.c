/*
 * Get the sequence number to use, and update it.
 */

#include	"defs.h"

int
get_seqno()
{
	int	seqno;
	FILE	*fp;

	if ( (fp = fopen(SEQNO_FILE, "r+")) == NULL)
		err_sys("can't open %s", SEQNO_FILE);

	my_lock(fileno(fp));		/* exclusive lock on file */

	if (fscanf(fp, "%d", &seqno) != 1)
		err_quit("fscanf error for sequence number");

	rewind(fp);
	fprintf(fp, "%03d\n", (seqno+1) % 1000 );	/* next seq# to use */
	fflush(fp);

	my_unlock(fileno(fp));		/* unlock file */

	fclose(fp);			/* and close it, we're done */

	return(seqno);
}
