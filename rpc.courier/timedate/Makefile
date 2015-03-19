#
BIN	= rdate dateserver
GEN	= Date1_defs.h Date1.h Date1_support.c Date1_server.c Date1_client.c
CLIOBJ	= rdate.o Date1_client.o Date1_support.o
SRVOBJ	= dateserver.o Date1_server.o Date1_support.o
LIB	= /tmp3/stevens/xns/lib/libcourier.a
RPCCOM	= /tmp3/stevens/xns/bin/xnscourier
CFLAGS	= -I/tmp3/stevens/xns/include

all: $(BIN)

$(GEN):		Date.cr
		$(RPCCOM) Date.cr

dateserver:	$(SRVOBJ)
		$(CC) -o $@ $(SRVOBJ) $(LIB)

rdate:		$(CLIOBJ)
		$(CC) -o $@ $(CLIOBJ) $(LIB)

$(CLIOBJ) $(SRVOBJ): Date1_defs.h

clean:
		rm -f $(GEN) *.o $(BIN)

