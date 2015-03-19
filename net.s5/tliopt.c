/*
 * Print the information for a given transport provider.
 */

#include	<stdio.h>
#include	<fcntl.h>
#include	<tiuser.h>

main()
{
	doit("/dev/tcp");
	doit("/dev/udp");
}

doit(name)
char	*name;
{
	int		tfd;
	struct t_info	info;

	if ( (tfd = t_open(name, O_RDWR, &info)) < 0)
		err_sys("can't open %s", name);

	printf("Protocol = %s", name);
	printf("\n  %8ld: max size of address", info.addr);
		if (info.addr == -2)
			printf(" (no user access to addresses)");
		else if (info.addr == -1)
			printf(" (no limit on size of address)");

	printf("\n  %8ld: max #bytes of options", info.options);
		if (info.options == -2)
			printf(" (not supported)");
		else if (info.options == -1)
			printf(" (no limit on option size)");

	printf("\n  %8ld: max size of data unit", info.tsdu);
		if (info.tsdu == -2)
			printf(" (transfer of normal data is not supported)");
		else if (info.tsdu == -1)
			printf(" (no limit on size of data unit)");
		else if (info.tsdu == 0)
			printf(" (data has no logical boundaries)");

	printf("\n  %8ld: max size of expedited data unit", info.etsdu);
		if (info.etsdu == -2)
			printf(" (transfer of normal data is not supported)");
		else if (info.etsdu == -1)
			printf(" (no limit on size of data unit)");
		else if (info.etsdu == 0)
			printf(" (data has no logical boundaries)");

	printf("\n  %8ld: max amount of data with connect", info.connect);
		if (info.connect == -2)
			printf(" (not allowed)");
		else if (info.connect == -1)
			printf(" (no limit)");

	printf("\n  %8ld: max amount of data with disconnect", info.discon);
		if (info.discon == -2)
			printf(" (not allowed)");
		else if (info.discon == -1)
			printf(" (no limit)");

	printf("\n  %8ld: service type", info.servtype);
		if (info.servtype == T_COTS)
			printf(" (connection-oriented withOUT orderly release)");
		else if (info.servtype == T_COTS_ORD)
			printf(" (connection-oriented WITH orderly release)");
		else if (info.servtype == T_CLTS)
			printf(" (connectionless)");
		else
			printf(" (??????)");
		putchar('\n');

	fflush(stdout);
}
