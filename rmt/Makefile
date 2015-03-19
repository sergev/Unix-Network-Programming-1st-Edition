#

CFLAGS	= -O
MYLIB	= ../libnet.a

all:	rmtd

#

rmtd:		rmtd.o $(MYLIB)
	cc $(CFLAGS) -o $@ rmtd.o $(MYLIB)

#
clean:
	-rm -f *.o core a.out temp*.* myrmt rmtd
