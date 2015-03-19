#

MYLIB	= ../libnet.a
LIBS	=
CFLAGS	= -O

all:	tcp udp idp spp unixstr unixdg sockopt hostent

#
# Internet stream version (TCP protocol).
#

tcp:	tcpserv tcpcli

tcpcli.o tcpserv.o: inet.h

tcpserv:	tcpserv.o strecho.o
		cc $(CFLAGS) -o $@ tcpserv.o strecho.o $(MYLIB) $(LIBS)

tcpcli:		tcpcli.o strcli.o
		cc $(CFLAGS) -o $@ tcpcli.o strcli.o $(MYLIB) $(LIBS)

#
# Internet datagram version (UDP protocol).
#

udp:	udpserv udpcli

udpcli.o udpserv.o: inet.h

udpserv:	udpserv.o dgecho.o
		cc $(CFLAGS) -o $@ udpserv.o dgecho.o $(MYLIB) $(LIBS)

udpcli:		udpcli.o dgcli.o
		cc $(CFLAGS) -o $@ udpcli.o dgcli.o $(MYLIB) $(LIBS)

#
# XNS stream version (SPP protocol).
#

spp:	sppserv sppcli

sppcli.o sppserv.o: xns.h

sppserv:	sppserv.o strecho.o
		cc $(CFLAGS) -o $@ sppserv.o strecho.o $(MYLIB) $(LIBS)

sppcli:		sppcli.o strcli.o
		cc $(CFLAGS) -o $@ sppcli.o strcli.o $(MYLIB) $(LIBS)

#
# XNS datagram version (IDP protocol).
#

idp:	idpserv idpcli

idpcli.o idpserv.o: xns.h

idpserv:	idpserv.o dgecho.o
		cc $(CFLAGS) -o $@ idpserv.o dgecho.o $(MYLIB) $(LIBS)

idpcli:		idpcli.o dgcli.o
		cc $(CFLAGS) -o $@ idpcli.o dgcli.o $(MYLIB) $(LIBS)

#
# UNIX stream version.
#

unixstr: unixstrserv unixstrcli

unixstrcli.o unixstrserv.o: unix.h

unixstrserv:	unixstrserv.o strecho.o
		cc $(CFLAGS) -o $@ unixstrserv.o strecho.o $(MYLIB) $(LIBS)

unixstrcli:	unixstrcli.o strcli.o
		cc $(CFLAGS) -o $@ unixstrcli.o strcli.o $(MYLIB) $(LIBS)

#
# UNIX datagram version.
#

unixdg:	unixdgserv unixdgcli

unixdgcli.o unixdgserv.o: unix.h

unixdgserv:	unixdgserv.o dgecho.o
		cc $(CFLAGS) -o $@ unixdgserv.o dgecho.o $(MYLIB) $(LIBS)

unixdgcli:	unixdgcli.o dgcli.o
		cc $(CFLAGS) -o $@ unixdgcli.o dgcli.o $(MYLIB) $(LIBS)

#
sockopt:	sockopt.o
		cc $(CFLAGS) -o $@ sockopt.o $(MYLIB) $(LIBS)

#
hostent:	hostent.o
		cc $(CFLAGS) -o $@ hostent.o $(MYLIB) $(LIBS)

#
clean:
	-rm -f *.o core a.out temp*.* \
		tcpserv tcpcli udpserv udpcli \
		sppserv sppcli idpserv idpcli \
		unixstrserv unixstrcli unixdgserv unixdgcli \
		s.unixdg s.unixstr \
		sockopt hostent
