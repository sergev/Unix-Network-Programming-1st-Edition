#
# Makefile for inettime program.
#

CFLAGS	= -O
OBJ	= main.o inettime.o
MYLIB	= ../libnet.a

all:
	make `systype.sh`

bsd:
	make "CFLAGS = " "LIBS = " inettime

sys5:
	make "CFLAGS = -I/usr/netinclude" \
		"LIBS = -lnet -lnsl_s" inettime

xenix:
	make "CFLAGS = -Ml -I/usr/include/exos" \
		"LIBS = -lsocket" inettime

inettime: $(OBJ) $(MYLIB)
	  cc -o inettime $(OBJ) $(MYLIB) $(LIBS)

clean:
	-rm -f *.o inettime *.out temp.* core
