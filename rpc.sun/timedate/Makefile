#
BIN = date_svc rdate
GEN = date_clnt.c date_svc.c date.h
LIB = ../librpclib.a
RPCCOM = /usr1/stevens/bin/rpcgen
CFLAGS = -I.. -O

all: $(BIN)

$(GEN):		date.x
		$(RPCCOM) date.x

date_svc:	date_proc.o date_svc.o
		$(CC) -o $@ date_proc.o date_svc.o $(LIB)

rdate:		rdate.o date_clnt.o
		$(CC) -o $@ rdate.o date_clnt.o $(LIB)

rdate.o:	rdate.c date.h

date_proc.o:	date_proc.c date.h

clean:
		rm -f $(GEN) *.o $(BIN)

