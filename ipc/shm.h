#include	"mesg.h"

#define	NBUFF	4	/* number of buffers in shared memory */
			/* (for multiple buffer version) */

#define	SHMKEY	((key_t) 7890) /* base value for shmem key */

#define	SEMKEY1	((key_t) 7891) /* client semaphore key */
#define	SEMKEY2	((key_t) 7892) /* server semaphore key */

#define	PERMS	0666
