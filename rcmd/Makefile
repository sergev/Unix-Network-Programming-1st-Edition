#

CFLAGS	= -O

all:	rshd rcmd.o

rshd:		rshd.o rresvport.o ruserok.o
	cc $(CFLAGS) -o $@ rshd.o rresvport.o ruserok.o

#
clean:
	rm -f *.o *.out temp.* core rshd
