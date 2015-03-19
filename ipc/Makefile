#
CFLAGS = -O

MYLIB	= ../libnet.a

#
# message version using shared memory and semaphore.  1-buffer.
# (No stream version for shared memory.)
#

m1_shm:		m1_shmserv m1_shmcli

m1_shmserv:	mainshmserv.o $(MYLIB)
	cc $(CFLAGS) -o server mainshmserv.o $(MYLIB)

m1_shmcli:	mainshmcli.o $(MYLIB)
	cc $(CFLAGS) -o client mainshmcli.o $(MYLIB)

#
# message version using shared memory and semaphore.  n-buffers.
# (No stream version for shared memory.)
#

mn_shm:		mn_shmserv mn_shmcli

mn_shmserv:	mainshmnserv.o $(MYLIB)
	cc $(CFLAGS) -o server mainshmnserv.o $(MYLIB)

mn_shmcli:	mainshmncli.o $(MYLIB)
	cc $(CFLAGS) -o client mainshmncli.o $(MYLIB)

#
# message version using 2 message queues
# (No stream version using message queues.)
#

m_msgq:		m_msgqserv m_msgqcli

m_msgqserv:	mainmsgqserv.o sub_servmesg.o sub_mesgmsgq.o $(MYLIB)
	cc $(CFLAGS) -o server mainmsgqserv.o sub_servmesg.o sub_mesgmsgq.o $(MYLIB)

m_msgqcli:	mainmsgqcli.o sub_climesg.o sub_mesgmsgq.o $(MYLIB)
	cc $(CFLAGS) -o client mainmsgqcli.o sub_climesg.o sub_mesgmsgq.o $(MYLIB)

#
# message version using 1 message queues
# (No stream version using message queues.)
#

m1_msgq:	m1_msgqserv m1_msgqcli

m1_msgqserv:	mainmsgqser1.o sub_mesgmsgq.o $(MYLIB)
	cc $(CFLAGS) -o server mainmsgqser1.o sub_mesgmsgq.o $(MYLIB)

m1_msgqcli:	mainmsgqcli1.o sub_mesgmsgq.o $(MYLIB)
	cc $(CFLAGS) -o client mainmsgqcli1.o sub_mesgmsgq.o $(MYLIB)

#
# message version using FIFO
#

m_fifo:		m_fifoserv m_fifocli

m_fifoserv:	mainfifoserv.o sub_servmesg.o sub_mesgmine.o $(MYLIB)
	cc $(CFLAGS) -o server mainfifoserv.o sub_servmesg.o sub_mesgmine.o $(MYLIB)

m_fifocli:	mainfifocli.o sub_climesg.o sub_mesgmine.o $(MYLIB)
	cc $(CFLAGS) -o client mainfifocli.o sub_climesg.o sub_mesgmine.o $(MYLIB)

#
# stream version using FIFO
#

s_fifo:		s_fifoserv s_fifocli

s_fifoserv:	mainfifoserv.o sub_servfd.o $(MYLIB)
	cc $(CFLAGS) -o server mainfifoserv.o sub_servfd.o $(MYLIB)

s_fifocli:	mainfifocli.o sub_clifd.o $(MYLIB)
	cc $(CFLAGS) -o client mainfifocli.o sub_clifd.o $(MYLIB)

#
# message version using pipes
# (The pipe version is a single program for client and server,
#  since a pipe can't be shared between unrelated processes.)
#

m_pipe:	mainpipefork.o sub_climesg.o sub_servmesg.o sub_mesgmine.o $(MYLIB)
	cc $(CFLAGS) -o mainpipe mainpipefork.o sub_climesg.o sub_servmesg.o sub_mesgmine.o $(MYLIB)

#
# stream version using pipes
# (The pipe version is a single program for client and server,
#  since a pipe can't be shared between unrelated processes.)
#

s_pipe:	mainpipefork.o sub_clifd.o sub_servfd.o $(MYLIB)
	cc $(CFLAGS) -o mainpipe mainpipefork.o sub_clifd.o sub_servfd.o $(MYLIB)

#
# Timing programs.
#

t_pipe:		mainpipefork.o tim_mesg1.o sub_mesgmine.o $(MYLIB)
	cc $(CFLAGS) -o t_pipe mainpipefork.o tim_mesg1.o sub_mesgmine.o $(MYLIB)

#

t_fifo:		t_fifocli t_fifoserv

t_fifocli:	mainfifocli.o tim_mesg1.o sub_mesgmine.o $(MYLIB)
	cc $(CFLAGS) -o t_fifocli mainfifocli.o tim_mesg1.o sub_mesgmine.o $(MYLIB)

t_fifoserv:	mainfifoserv.o tim_mesg1.o sub_mesgmine.o $(MYLIB)
	cc $(CFLAGS) -o t_fifoserv mainfifoserv.o tim_mesg1.o sub_mesgmine.o $(MYLIB)

#

t_msgq:		t_msgqcli t_msgqserv

t_msgqcli:	mainmsgqcli.o tim_mesg1.o sub_mesgmsgq.o $(MYLIB)
	cc $(CFLAGS) -o t_msgqcli mainmsgqcli.o tim_mesg1.o sub_mesgmsgq.o $(MYLIB)

t_msgqserv:	mainmsgqserv.o tim_mesg1.o sub_mesgmsgq.o $(MYLIB)
	cc $(CFLAGS) -o t_msgqserv mainmsgqserv.o tim_mesg1.o sub_mesgmsgq.o $(MYLIB)

#

t_shm:		t_shmserv t_shmcli

t_shmserv:	tim_shmserv.o $(MYLIB)
	cc $(CFLAGS) -o t_shmserv tim_shmserv.o $(MYLIB)

t_shmcli:	tim_shmcli.o $(MYLIB)
	cc $(CFLAGS) -o t_shmcli tim_shmcli.o $(MYLIB)

#

ti_all:		ti_pipe ti_fifo ti_msgq ti_sem1 ti_sem2 ti_getpid

ti_pipe:	time_pipe.o $(MYLIB)
	cc $(CFLAGS) -o ti_pipe time_pipe.o $(MYLIB)

ti_fifo:	time_fifo.o $(MYLIB)
	cc $(CFLAGS) -o ti_fifo time_fifo.o $(MYLIB)

ti_msgq:	time_msgq.o $(MYLIB)
	cc $(CFLAGS) -o ti_msgq time_msgq.o $(MYLIB)

ti_sem1:	time_sem1.o $(MYLIB)
	cc $(CFLAGS) -o ti_sem1 time_sem1.o $(MYLIB)

ti_sem2:	time_sem2.o $(MYLIB)
	cc $(CFLAGS) -o ti_sem2 time_sem2.o $(MYLIB)

ti_getpid:	time_getpid.o $(MYLIB)
	cc $(CFLAGS) -o ti_getpid time_getpid.o $(MYLIB)

#
# Ancillary programs.
#

getpwd:	getpwd.o $(MYLIB)
	cc $(CFLAGS) -o getpwd getpwd.o $(MYLIB)

mainpopen:	mainpopen.o $(MYLIB)
	cc $(CFLAGS) -o mainpopen mainpopen.o $(MYLIB)

clean:
	-rm -f *.o client server mainpipe getpwd mainpopen core a.out \
		temp.* t_* ti_*
