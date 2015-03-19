#

MYLIB	= ../libnet.a

all:	readnspipe

#

readnspipe: readnspipe.o $(MYLIB)
	    cc $(CFLAGS) -o $@ readnspipe.o $(MYLIB)

#
clean:
	-rm -f *.o core a.out temp*.* readnspipe
