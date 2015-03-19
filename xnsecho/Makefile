#

CFLAGS	= -O
OBJ	= xnsecho.o
MYLIB	= ../libnet.a

all:	xnsecho

#

xnsecho: $(OBJ) $(MYLIB)
	 cc $(CFLAGS) -o $@ $(OBJ) $(MYLIB)

#
clean:
	-rm -f *.o core a.out temp*.* xnsecho
