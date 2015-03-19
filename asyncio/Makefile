#

CFLAGS	= -O
MYLIB	= ../libnet.a

all:	oldcat aiocat

#

oldcat:	oldcat.o $(MYLIB)
	cc $(CFLAGS) -o $@ oldcat.o $(MYLIB)

aiocat: aiocat.o $(MYLIB)
	cc $(CFLAGS) -o $@ aiocat.o $(MYLIB)

#
clean:
	-rm -f *.o core a.out temp*.* oldcat aiocat
