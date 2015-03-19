 # (leading space for Xenix /bin/sh)

SYS=`systype.sh`

case $SYS in
bsd)	LIST="ti_pipe ti_fifo ti_msgq"
	CF="CFLAGS=-O" ;;

xenix)	LIST="ti_pipe ti_fifo ti_msgq"
	CF="CFLAGS=-Me -Ml -O" ;;

unixpc)	LIST="ti_pipe ti_fifo ti_msgq"
	CF="CFLAGS=-O" ;;

*)	echo "Unknown system type" ; exit 1 ;;
esac

echo $SYS

#
# The getpid() and semaphore tests only need to be run once since
# they don't change with different buffer lengths.
#

eval make "'${CF}'" clean ti_getpid

echo " "
echo "++++++++++ getpid, len = $LEN"
time ti_getpid
time ti_getpid
time ti_getpid

if test -f /usr/include/sys/sem.h
then
	eval make "'${CF}'" ti_sem1 ti_sem2

	echo " "
	echo "++++++++++ sem1, len = $LEN"
	time ti_sem1
	time ti_sem1
	time ti_sem1

	echo " "
	echo "++++++++++ sem2, len = $LEN"
	time ti_sem2
	time ti_sem2
	time ti_sem2
fi

#
# Once for each buffer length.
#

for LEN in 32 128 512 2048
do
	echo
	echo "**********************  LEN = ${LEN}  **********************"
	for i in time_*.c
	do
		ex $i <<EOF
g/^#define	BUFFSIZE/s/	[0-9][0-9]*$/	${LEN}/
x
EOF
	done
	
	eval make "'${CF}'" $LIST
	
	echo " "
	echo "++++++++++ pipe, len = $LEN"
	time ti_pipe
	time ti_pipe
	time ti_pipe
	
	echo " "
	echo "++++++++++ fifo, len = $LEN"
	time ti_fifo
	time ti_fifo
	time ti_fifo
	
	echo " "
	echo "++++++++++ message queue, len = $LEN"
	time ti_msgq
	time ti_msgq
	time ti_msgq
done
exit 0
