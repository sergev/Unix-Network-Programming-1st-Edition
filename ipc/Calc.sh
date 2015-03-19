#! /bin/sh

# Number of times each program was timed
Nruns=3

# Number of loops in each program
Nloop=20000

File=/tmp/my$$

cat $1 > $File

#
# Copy the file above, then edit the file (to join lines).
# The 3b1 puts out times of the form "1m8.76s".  Convert this to "1:8.76"
# then process as a Xenix date.
#

ex - $File <<MYEOF
g/^real.*[0-9]*m[0-9]*.[0-9]*s/s/m/:/
g/^real.*[0-9]*:[0-9]*.[0-9]*s/s/s//
g/^user.*[0-9]*m[0-9]*.[0-9]*s/s/m/:/
g/^user.*[0-9]*:[0-9]*.[0-9]*s/s/s//
g/^sys.*[0-9]*m[0-9]*.[0-9]*s/s/m/:/
g/^sys.*[0-9]*:[0-9]*.[0-9]*s/s/s//
g/^real/j2
g/++++++++++/s///
g/message queue,/s//mque,/
g/shared memory,/s//shm, /
x
MYEOF

nawk '

function cvttime(str) {
	#
	# Xenix puts out times such as "1:14.6" for 74.6 seconds
	#
	if (index(str, ":") > 0) {
		i = split(str, arr, ":")
		if (i != 2) {
			print "split error"
			exit 1
		}
		return(arr[1]*60 + arr[2])
	} else {
		return(str)
	}
}

/len = /     { title = $0 }

$2 == "real" {	# VAX line is:  I real J user K sys
		# times are of the form 74.6, for example
		counter += 1
		real += $1
		cpu  += $3 + $5
		if (counter == nruns) {
			printf("real = %5.1f, cpu = %5.1f, mesg/sec = %6d: %s\n", real/nruns, cpu/nruns, (nruns*nloop)/real, title)
			real = 0
			cpu  = 0
			counter = 0
		}
	}

$1 == "real" {	# PC/AT & UNIX PC line is:  real I user J sys K
		# times are of the form 1:14.6, for example.
		counter += 1
		real += cvttime($2)
		cpu  += cvttime($4) + cvttime($6)
		if (counter == nruns) {
			printf("real = %5.1f, cpu = %5.1f, mesg/sec = %6d: %s\n", real/nruns, cpu/nruns, (nruns*nloop)/real, title)
			real = 0
			cpu  = 0
			counter = 0
		}
	}' nruns=${Nruns} nloop=${Nloop} ${File}

rm ${File}
