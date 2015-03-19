#
# Makefile for ping program.
#

MYLIB	= ../libnet.a
OBJ	= checksum.o finish.o main.o print.o recvping.o \
		sendping.o sigalarm.o

all:
	make `systype.sh`

bsd:
	make "CFLAGS = -O" "LIBS = " ping

ping:	$(OBJ)
	cc -o ping $(CFLAGS) $(OBJ) $(MYLIB) $(LIBS)

$(OBJ): defs.h

clean:
	rm -f *.o ping *.out.* temp.* make.* Make.* core
