#
# Makefile for locking tests.
#

MYLIB	= ../libnet.a

all:
	make `systype.sh`

bsd:
	make "CFLAGS = " "LIBS = " \
			lockbsd lockcreat locknone lockopen lockv7

sys5:
	make "CFLAGS = -I/usr/netinclude" \
		"LIBS = -lnet -lnsl_s" \
			lockmainsem lockcreat locknone lockopen locks5 \
			locksem locksem1 locksem2 lockv7 mandatory

xenix:
	make "CFLAGS = -Ml -I/usr/include/exos" \
		"LIBS = -lsocket" \
			lockmainsem lockcreat locknone lockopen locks5 \
			locksem locksem1 locksem2 lockv7

lockmainsem:	lockmainsem.o $(MYLIB)
	cc $(CFLAGS) -o $@ lockmainsem.o $(MYLIB) $(LIBS)

lockbsd:	lockbsd.o lockmain.o $(MYLIB)
	cc $(CFLAGS) -o $@ lockmain.o lockbsd.o $(MYLIB) $(LIBS)

lockcreat:	lockcreat.o lockmain.o $(MYLIB)
	cc $(CFLAGS) -o $@ lockmain.o lockcreat.o $(MYLIB) $(LIBS)

locknone:	locknone.o lockmain.o $(MYLIB)
	cc $(CFLAGS) -o $@ lockmain.o locknone.o $(MYLIB) $(LIBS)

lockopen:	lockopen.o lockmain.o $(MYLIB)
	cc $(CFLAGS) -o $@ lockmain.o lockopen.o $(MYLIB) $(LIBS)

locks5:		locks5.o lockmain.o $(MYLIB)
	cc $(CFLAGS) -o $@ lockmain.o locks5.o $(MYLIB) $(LIBS)

locksem:	locksem.o lockmain.o $(MYLIB)
	cc $(CFLAGS) -o $@ lockmain.o locksem.o $(MYLIB) $(LIBS)

locksem1:	locksem1.o lockmain.o $(MYLIB)
	cc $(CFLAGS) -o $@ lockmain.o locksem1.o $(MYLIB) $(LIBS)

locksem2:	locksem2.o lockmain.o $(MYLIB)
	cc $(CFLAGS) -o $@ lockmain.o locksem2.o $(MYLIB) $(LIBS)

lockv7:		lockv7.o lockmain.o $(MYLIB)
	cc $(CFLAGS) -o $@ lockmain.o lockv7.o $(MYLIB) $(LIBS)

#
# Special program to test System V mandatory versus advisory locking.
#
mandatory:	mandatory.o $(MYLIB)
	cc $(CFLAGS) -o $@ mandatory.o $(MYLIB) $(LIBS)

#
clean:
	rm -f *.o a.out temp.* core lockbsd lockcreat locknone lockopen \
		locks5 locksem locksem1 locksem2 \
		lockv7 mandatory lockmainsem
