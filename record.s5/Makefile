#

MYLIB	= ../libnet.a

all:	record1 record2 record3 record4

#
record1: main1.o parent1.o child1.o $(MYLIB)
	 cc $(CFLAGS) -o $@ main1.o parent1.o child1.o $(MYLIB)
#
record2: main2.o parent1.o child1.o $(MYLIB)
	 cc $(CFLAGS) -o $@ main2.o parent1.o child1.o $(MYLIB) -lpt
#
record3: main3.o parent1.o child1.o $(MYLIB)
	 cc $(CFLAGS) -o $@ main3.o parent1.o child1.o $(MYLIB) -lpt
#
record4: main4.o parent1.o child1.o $(MYLIB)
	 cc $(CFLAGS) -o $@ main4.o parent1.o child1.o $(MYLIB) -lpt
#
clean:
	-rm -f *.o core a.out temp*.* record1 record2 record3 record4
