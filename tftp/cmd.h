/*
 * Header file for user command processing functions.
 */

#include	"defs.h"

extern char	temptoken[];	/* temporary token for anyone to use */

typedef struct Cmds {
  char	*cmd_name;		/* actual command string */
  int	(*cmd_func)();		/* pointer to function */
} Cmds;

extern Cmds	commands[];
extern int	ncmds;		/* number of elements in array */
