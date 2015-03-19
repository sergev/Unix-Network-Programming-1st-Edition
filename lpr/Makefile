#
# Makefile for line printer spooler client program.
#

OBJ	= main.o initvars.o printbsd.o seqno.o
SRC	= main.c initvars.c printbsd.c seqno.c
MYLIB	= ../libnet.a

all:
	make `systype.sh`

bsd:
	make "CFLAGS = -O" \
		"LIBS = " mylpr

sys5:
	make "CFLAGS = -I/usr/netinclude -O" \
		"LIBS = -lnet -lnsl_s" mylpr

mylpr:	$(OBJ)
	cc -o mylpr $(CFLAGS) $(OBJ) $(MYLIB) $(LIBS)

$(OBJ): defs.h

clean:
	rm -f *.o mylpr *.out temp.* make.* Make.* core cf[A-Z][0-9][0-9][0-9]*

print:	clean
	print *.h *.c

lint:
	lint -bhx $(SRC) > lint.out
