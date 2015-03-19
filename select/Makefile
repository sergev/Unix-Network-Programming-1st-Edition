#

CFLAGS	= -O
MYLIB	= ../libnet.a

all:	timer

#

timer:	timer.o $(MYLIB)
	cc $(CFLAGS) -o $@ timer.o $(MYLIB)

#
clean:
	rm -f *.o *.out temp.* core timer
