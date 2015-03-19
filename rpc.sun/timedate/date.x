/*
 * date.x - Specification of remote date and time service.
 */

/*
 * Define 2 procedures:
 *	bin_date_1() returns the binary time and date (no arguments).
 *	str_date_1() takes a binary time and returns a human-readable string.
 */

program DATE_PROG {
    version DATE_VERS {
	long	BIN_DATE(void) = 1;	/* procedure number = 1 */
	string	STR_DATE(long) = 2;	/* procedure number = 2 */
    } = 1;				/* version number = 1 */
} = 0x31234567;				/* program number = 0x31234567 */
