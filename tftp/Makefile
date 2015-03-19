#
# Makefile for tftp client and server
#
#	This Makefile uses the files "make.client" and "make.server"
#	to remember whether the client or server was last made.
#	This way, you might not have to recompile everything each time.
#	However, when switching from the client to the server, or vice-versa,
#	you have to remake everything, since there are #ifdef's in each C
#	file for the client or server.  The resulting object files can't
#	be reused from the client to the server, or vice-versa.
#

CFLAGS	= -O
LIBS    =
MYLIB	= ../libnet.a

CMDOBJ	= cmd.o cmdgetput.o cmdsubr.o	# only the clients process commands
CMDSRC	= cmd.c cmdgetput.c cmdsubr.c

CLIOBJ	= error.o file.o fsm.o initvars.o \
	  maincli.o sendrecv.o $(CMDOBJ)
CLISRC	= error.c file.c fsm.c initvars.c \
	  maincli.c sendrecv.c $(CMDSRC)

SRVOBJ	= error.o file.o fsm.o initvars.o \
	  mainserv.o sendrecv.o
SRVSRC	= error.c file.c fsm.c initvars.c \
	  mainserv.c sendrecv.c

all:
	make `systype.sh`udpclient
	make `systype.sh`udpserver

udp:
	make `systype.sh`udpclient
	make `systype.sh`udpserver

tcp:
	make `systype.sh`tcpclient
	make `systype.sh`tcpserver

udpclient:
	make `systype.sh`udpclient

udpserver:
	make `systype.sh`udpserver

tcpclient:
	make `systype.sh`tcpclient

tcpserver:
	make `systype.sh`tcpserver

bsdudpclient:
	-@if [ -r make.server ] ; \
	then \
		rm -f $(CLIOBJ) netudp.o make.server ; \
	else \
		true ; \
	fi
	@touch make.client
	@make "CFLAGS=-O -DCLIENT -DDATAGRAM" \
		"NETOBJ=netudp.o" "LIBS = " cli_tftp

bsdudpserver:
	-@if [ -r make.client ] ; \
	then \
		rm -f $(SRVOBJ) netudp.o make.client ; \
	else \
		true ; \
	fi
	@touch make.server
	@make "CFLAGS=-O -DSERVER -DDATAGRAM" \
		"NETOBJ=netudp.o" "LIBS = " srv_tftp

bsdtcpclient:
	-@if [ -r make.server ] ; \
	then \
		rm -f $(CLIOBJ) nettcp.o make.server ; \
	else \
		true ; \
	fi
	@touch make.client
	@make "CFLAGS=-O -DCLIENT" \
		"NETOBJ=nettcp.o" "LIBS = " cli_tftp

bsdtcpserver:
	-@if [ -r make.client ] ; \
	then \
		rm -f $(SRVOBJ) nettcp.o make.client ; \
	else \
		true ; \
	fi
	@touch make.server
	@make "CFLAGS=-O -DSERVER" \
		"NETOBJ=nettcp.o" "LIBS = " srv_tftp

sys5udpclient:
	-@if [ -r make.server ] ; \
	then \
		rm -f $(CLIOBJ) netudp.o make.server ; \
	else \
		true ; \
	fi
	@touch make.client
	@make "CFLAGS = -O -DCLIENT -DDATAGRAM -I/usr/netinclude" \
		"NETOBJ=netudp.o" "LIBS = -lnet -lnsl_s" cli_tftp

sys5udpserver:
	-@if [ -r make.client ] ; \
	then \
		rm -f $(SRVOBJ) netudp.o make.client ; \
	else \
		true ; \
	fi
	@touch make.server
	@make "CFLAGS = -O -DSERVER -DDATAGRAM -I/usr/netinclude" \
		"NETOBJ=netudp.o" "LIBS = -lnet -lnsl_s" srv_tftp

xenixudpclient:
	-@if [ -r make.server ] ; \
	then \
		rm -f $(CLIOBJ) netudp.o make.server ; \
	else \
		true ; \
	fi
	@touch make.client
	@make "CFLAGS = -O -DCLIENT -DDATAGRAM -Ml -I/usr/include/exos" \
		"NETOBJ=netudp.o" "LIBS = -lsocket" cli_tftp

xenixudpserver:
	-@if [ -r make.client ] ; \
	then \
		rm -f $(SRVOBJ) netudp.o make.client ; \
	else \
		true ; \
	fi
	@touch make.server
	@make "CFLAGS = -O -DSERVER -DDATAGRAM -Ml -I/usr/include/exos" \
		"NETOBJ=netudp.o" "LIBS = -lsocket" srv_tftp

cli_tftp: $(CLIOBJ) $(NETOBJ) $(MYLIB)
	  cc -o tftp $(CFLAGS) $(CLIOBJ) $(NETOBJ) $(MYLIB) $(LIBS)

srv_tftp: $(SRVOBJ) $(NETOBJ) $(MYLIB)
	  cc -o tftpserv $(CFLAGS) $(SRVOBJ) $(NETOBJ) $(MYLIB) $(LIBS)

$(CLIOBJ): defs.h
$(SRVOBJ): defs.h
$(CMDOBJ): cmd.h

clean:
	rm -f *.o tftp tftpserv *.out temp.* make.* Make.* core

print:	clean
	print *.h *.c

lint:
	lint -DCLIENT -bhx $(CLISRC) >  lint.out
	lint -DSERVER -bhx $(SRVSRC) >> lint.out
