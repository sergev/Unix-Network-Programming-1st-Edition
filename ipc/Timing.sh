 # (leading space for Xenix /bin/sh)

SYS=`systype.sh`

case $SYS in
bsd)	LIST="t_pipe t_fifo t_msgq"
	CF="CFLAGS=-O" ;;

xenix)	LIST="t_pipe t_fifo t_msgq t_shm"
	CF="CFLAGS=-Me -Ml -O" ;;

unixpc)	LIST="t_pipe t_fifo t_msgq t_shm"
	CF="CFLAGS=-O" ;;

*)	echo "Unknown system type" ; exit 1 ;;
esac

for LEN in 32 128 512 2048
do
	echo
	echo "**********************  LEN = ${LEN}  **********************"
	for i in tim_*.c
	do
		ex $i <<EOF
g/^#define	MESGLEN/s/	[0-9][0-9]*$/	${LEN}/
x
EOF
	done
	
	eval make "'${CF}'" $LIST
	
	echo " "
	echo "++++++++++ pipe, len = $LEN"
	time t_pipe
	time t_pipe
	time t_pipe
	
	echo " "
	echo "++++++++++ fifo, len = $LEN"
	t_fifoserv &
		sleep 2
	time t_fifocli
	t_fifoserv &
		sleep 2
	time t_fifocli
	t_fifoserv &
		sleep 2
	time t_fifocli
	
	echo " "
	echo "++++++++++ message queue, len = $LEN"
	t_msgqserv &
		sleep 2
	time t_msgqcli
	t_msgqserv &
		sleep 2
	time t_msgqcli
	t_msgqserv &
		sleep 2
	time t_msgqcli
	
	if test -f /usr/include/sys/shm.h
	then
		echo " "
		echo "++++++++++ shared memory, len = $LEN"
		t_shmserv &
			sleep 2
		time t_shmcli
		t_shmserv &
			sleep 2
		time t_shmcli
		t_shmserv &
			sleep 2
		time t_shmcli
	fi
done
exit 0
