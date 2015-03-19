#
# Makefile for file descriptor passing example.  Note that the same Makefile
# can be used for both 4.3BSD and System V.  The only differences in the
# two examples are the files "sendfile.c" and "recvfile.c" along with the
# different MYLIB contents.
#

CFLAGS	= -O
MYLIB	= ../libnet.a

all:	mycat openfile

#

mycat:	mycat.o myopen.o recvfile.o $(MYLIB)
	cc $(CFLAGS) -o $@ mycat.o myopen.o recvfile.o $(MYLIB)

openfile: openfile.o sendfile.o $(MYLIB)
	  cc $(CFLAGS) -o $@ openfile.o sendfile.o $(MYLIB)

#
clean:
	-rm -f *.o core a.out temp*.* mycat openfile
