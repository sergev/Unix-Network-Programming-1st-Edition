 # (leading space required for Xenix /bin/sh)

#
# Determine the type of *ix operating system that we're
# running on, and echo an appropriate value.
# This script is intended to be used in Makefiles.
# (This is a kludge.  Gotta be a better way.)
#

if (test -f /vmunix)
then
	echo "bsd"
elif (test -f /xenix)
then
	echo "xenix"
elif (test -c /dev/spx)
then
	echo "sys5"
elif (test -f /etc/lddrv)
then
	echo "unixpc"
fi
