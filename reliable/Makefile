#

MYLIB	= ../libnet.a
CFLAGS	= -O

all:	rudp

#
# Reliable Internet datagram version (UDP protocol).
#

rudp:	rudpserv rudpcli

rudpcli.o rudpserv.o: rudp.h

rudpserv:	rudpserv.o $(MYLIB)
		cc $(CFLAGS) -o $@ rudpserv.o $(MYLIB)

rudpcli:	rudpcli.o $(MYLIB)
		cc $(CFLAGS) -o $@ rudpcli.o $(MYLIB)

#
clean:
	-rm -f *.o core a.out temp*.* rudpserv rudpcli
