/*
 * Definition of "our" message.
 *
 * You may have to change the 4096 to a smaller value, if message queues
 * on your system were configured with "msgmax" less than 4096.
 */

#define	MAXMESGDATA	(4096-16)
				/* we don't want sizeof(Mesg) > 4096 */

#define	MESGHDRSIZE	(sizeof(Mesg) - MAXMESGDATA)
				/* length of mesg_len and mesg_type */

typedef struct {
  int	mesg_len;	/* #bytes in mesg_data, can be 0 or > 0 */
  long	mesg_type;	/* message type, must be > 0 */
  char	mesg_data[MAXMESGDATA];
} Mesg;
