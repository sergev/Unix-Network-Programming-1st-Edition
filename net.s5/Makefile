#

MYLIB	= ../libnet.a
LIBS	= -lnet -lnsl_s
CFLAGS	= -I/usr/netinclude -g

all:	tcp udp

#
# Internet stream version (TCP protocol).
#

tcp:	tcpserv tcpcli

tcpcli.o tcpserv.o: inet.h

tcpserv:	tcpserv.o acceptcall.o strecho.o
		cc $(CFLAGS) -o $@ tcpserv.o acceptcall.o strecho.o $(MYLIB) $(LIBS)

tcpcli:		tcpcli.o
		cc $(CFLAGS) -o $@ tcpcli.o $(MYLIB) $(LIBS)

#
# Internet datagram version (UDP protocol).
#

udp:	udpserv udpcli

udpcli.o udpserv.o: inet.h

udpserv:	udpserv.o
		cc $(CFLAGS) -o $@ udpserv.o $(MYLIB) $(LIBS)

udpcli:		udpcli.o
		cc $(CFLAGS) -o $@ udpcli.o $(MYLIB) $(LIBS)

#
tliopt:		tliopt.o
		cc $(CFLAGS) -o $@ tliopt.o $(MYLIB) $(LIBS)

#
streammod:	streammod.o
		cc $(CFLAGS) -o $@ streammod.o $(MYLIB) $(LIBS)

#
hostent:	hostent.o
		cc $(CFLAGS) -o $@ hostent.o $(MYLIB) $(LIBS)

#
clean:
	-rm -f *.o core a.out temp*.* \
		tcpserv tcpcli udpserv udpcli \
		tliopt hostent
