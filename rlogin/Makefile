#
CFLAGS = -O

all:	rlogin rlogind

rlogin:  rlogin.o rcmd.o rresvport.o
	 cc -o rlogin rlogin.o rcmd.o rresvport.o

rlogind: rlogind.o logout.o logwtmp.o
	 cc -o rlogind rlogind.o logout.o logwtmp.o

rlogind.o:
	cc -c $(CFLAGS) -DOLD_LOGIN rlogind.c

# must be root to install
install:
	cp ./rlogin       /usr/local/bin/xrlogin
	cp ./rlogind      /usr/local/bin/xrlogind
	/etc/chown root   /usr/local/bin/xrlogin /usr/local/bin/xrlogind
	chgrp staff       /usr/local/bin/xrlogin /usr/local/bin/xrlogind
	chmod g-w         /usr/local/bin/xrlogin
	chmod u+s         /usr/local/bin/xrlogin
	ls -lg            /usr/local/bin/xrlogin /usr/local/bin/xrlogind
	rcp ./rlogin          hsi:/usr/local/bin/xrlogin
	rcp ./rlogind         hsi:/usr/local/bin/xrlogind
	rsh hsi "/etc/chown root /usr/local/bin/xrlogin /usr/local/bin/xrlogind"
	rsh hsi "chgrp staff     /usr/local/bin/xrlogin /usr/local/bin/xrlogind"
	rsh hsi "chmod g-w       /usr/local/bin/xrlogin"
	rsh hsi "chmod u+s       /usr/local/bin/xrlogin"
	rsh hsi "ls -lg          /usr/local/bin/xrlogin /usr/local/bin/xrlogind"

clean:
	rm -f *.o rlogin rlogind *.out temp.* make.* core
